#include "server.h"
#include "utils.h"
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct Host {
  char name[NAME_SZ]; // vulnerable buffer
  void (*print_fn)(char *,
                   struct Host *); // function pointer we allow overwrite of
  char token[TOKEN_SZ];            // registration token (hex)
  double cpu;                      // data points
  double ram;                      // data points
};

static struct Host *monitors[MAX_MON];
static int mon_count = 0;
static char admin_token[ADMIN_TOKEN_SZ];

/*
 * Read from env ADMIN_TOKEN if set, otherwise generate random
 */
static void read_admin_token(char *buf) {
  char *env = getenv("ADMIN_TOKEN");
  if (env && strlen(env) < ADMIN_TOKEN_SZ) {
    strncpy(buf, env, ADMIN_TOKEN_SZ - 1);
    buf[ADMIN_TOKEN_SZ - 1] = '\0';
  } else {
    printf("ADMIN_TOKEN not set, quitting\n");
    exit(1);
  }
}

/*
 * Get host view string
 */
static void get_host_view(char *buf, struct Host *h) {
  snprintf(buf, MAX_VIEW_LEN, "HOST %s CPU %.2f RAM %.2f\n", h->name, h->cpu,
           h->ram);
}

/*
 * Find host by name, return index or -1 if not found
 */
static int find_host_by_name(const char *name) {
  for (int i = 0; i < mon_count; ++i) {
    if (monitors[i] && monitors[i]->name[0] != '\0' &&
        strcmp(monitors[i]->name, name) == 0)
      return i;
  }
  return -1;
}

/*
 * Add a new host monitor
 */
static void add_host(const char *name, const char *tok) {
  if (mon_count >= MAX_MON)
    return;
  struct Host *h = malloc(sizeof(struct Host));
  if (!h)
    return;
  memset(h, 0, sizeof(*h));
  strncpy(h->name, name, NAME_SZ - 1);
  h->name[NAME_SZ - 1] = 0;
  h->print_fn = get_host_view;
  if (tok)
    strncpy(h->token, tok, TOKEN_SZ - 1);
  h->cpu = 0.0;
  h->ram = 0.0;
  monitors[mon_count++] = h;
}

/*
 * Process a single command line from client_fd (line includes '\n').
 */
static void process_command(int client_fd, const char *line) {
  // Make local copy because strtok modifies string
  char buf[MAX_LINE];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  // remove trailing newline
  size_t L = strlen(buf);
  if (L && buf[L - 1] == '\n')
    buf[L - 1] = '\0';

  char *cmd = strtok(buf, " ");
  if (!cmd) {
    send_line_fd(client_fd, "ERR\n");
    return;
  }
  printf("Handling '[%s]'\n", cmd);

  if (strcasecmp(cmd, "REGISTER") == 0) {
    /* REGISTER <hlen>\n  then read hlen bytes (hostname) */
    char *hlen_s = strtok(NULL, " ");
    if (!hlen_s) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }
    long hlen = atol(hlen_s);
    if (hlen <= 0 || hlen >= NAME_SZ) {
      send_line_fd(client_fd, "BADLEN\n");
      return;
    }
    char *name = malloc(hlen + 1);
    if (!name) {
      send_line_fd(client_fd, "OOM\n");
      return;
    }
    if (read_n_fd(client_fd, name, (size_t)hlen) <= 0) {
      free(name);
      return;
    }
    name[hlen] = '\0';
    printf("REGISTER hostname='%s'\n", name);
    // consume possible newline after payload (non-critical)
    char ch;
    recv(client_fd, &ch, 1, MSG_DONTWAIT);

    if (find_host_by_name(name) != -1) {
      printf("REGISTER name exists\n");
      send_line_fd(client_fd, "EXISTS\n");
      free(name);
      return;
    }
    char tok[TOKEN_SZ];
    gen_hex_random(tok, 12);
    add_host(name, tok);
    char out[128];
    snprintf(out, sizeof(out), "TOKEN %s\n", tok);
    send(client_fd, out, strlen(out), 0);
    printf("REGISTER done\n");
    free(name);

  } else if (strcasecmp(cmd, "UPDATE") == 0) {
    /* UPDATE <hostname> <token> <cpu> <ram> */
    char *name = strtok(NULL, " ");
    char *tok = strtok(NULL, " ");
    char *cpu_s = strtok(NULL, " ");
    char *ram_s = strtok(NULL, " ");
    if (!name || !tok || !cpu_s || !ram_s) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }
    int idx = find_host_by_name(name);
    if (idx == -1) {
      printf("UPDATE no such host\n");
      send_line_fd(client_fd, "NOHOST\n");
      return;
    }
    struct Host *h = monitors[idx];
    if (!h) {
      send_line_fd(client_fd, "ERRNULL\n");
      return;
    }
    if (strcmp(h->token, tok) != 0) {
      send_line_fd(client_fd, "BADTOK\n");
      return;
    }
    double cpu = atof(cpu_s);
    double ram = atof(ram_s);
    h->cpu = cpu;
    h->ram = ram;
    send_line_fd(client_fd, "OK\n");
    printf("UPDATE done\n");

  } else if (strcasecmp(cmd, "EDIT") == 0) {
    /* EDIT <hostname> <token> <len>\n edit the hostname (vuln) */
    char *hostname = strtok(NULL, " ");
    if (!hostname) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }
    char *token = strtok(NULL, " ");
    if (!token) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }

    char *len_s = strtok(NULL, " ");
    if (!len_s) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }

    char *idx_s = hostname;
    int idx = find_host_by_name(idx_s);

    if (idx == -1) {
      send_line_fd(client_fd, "NOHOST\n");
      return;
    }

    struct Host *h = monitors[idx];

    if (strcmp(token, h->token) != 0) {
      send_line_fd(client_fd, "BADTOK\n");
      return;
    }

    long len = atol(len_s);
    if (idx < 0 || idx >= mon_count || len <= 0 || len > 4096) {
      send_line_fd(client_fd, "BAD\n");
      // read and discard payload to keep stream consistent
      char *tmp = malloc(len > 0 ? len : 1);
      if (tmp) {
        read_n_fd(client_fd, tmp, len > 0 ? len : 0);
        free(tmp);
      }
      return;
    }

    if (!h) {
      send_line_fd(client_fd, "NULL\n");
      // read payload and discard
      char *tmp = malloc(len);
      if (tmp) {
        read_n_fd(client_fd, tmp, len);
        free(tmp);
      }
      return;
    }
    char *buf = malloc((size_t)len);
    if (!buf) {
      send_line_fd(client_fd, "OOM\n");
      return;
    }
    if (read_n_fd(client_fd, buf, (size_t)len) <= 0) {
      free(buf);
      return;
    }
    // *** VULNERABLE COPY: no bounds check -> overflow from name into print_fn
    // is possible
    memcpy(h->name, buf, (size_t)len);
    // fill with zeros the rest of the name
    if (len < NAME_SZ) {
      memset(h->name + len, 0, NAME_SZ - len);
    }
    free(buf);
    send_line_fd(client_fd, "EDITOK\n");
    printf("EDIT done\n");

  } else if (strcasecmp(cmd, "VIEW") == 0) {
    /* VIEW <hostname> <token> -> call the host's print_fn (may have been
     * overwritten) */
    char *host = strtok(NULL, " ");
    if (!host) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }

    // get id of host
    int idx = find_host_by_name(host);
    if (idx == -1) {
      send_line_fd(client_fd, "NOHOST\n");
      return;
    }

    char *token = strtok(NULL, " ");
    if (!token) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }

    struct Host *h = monitors[idx];

    if (strcmp(token, h->token) != 0) {
      send_line_fd(client_fd, "BADTOK\n");
      return;
    }

    if (h->print_fn) {
      char buf[MAX_VIEW_LEN];
      h->print_fn(buf, h);
      send_line_fd(client_fd, buf);
      send_line_fd(client_fd, "VIEWOK\n");
    } else {
      send_line_fd(client_fd, "NOFN\n");
    }
    printf("VIEW done\n");

  } else if (strcasecmp(cmd, "DUMP") == 0) {
    /* ADMIN <token> authenticates admin for LIST */
    char *tok = strtok(NULL, " ");
    if (!tok) {
      send_line_fd(client_fd, "BAD\n");
      return;
    }
    if (strcmp(tok, admin_token) == 0) {
      printf("ADMIN auth success\n");

      char out[256];
      for (int i = 0; i < mon_count; ++i) {
        struct Host *h = monitors[i];
        h->print_fn(out, h);

        printf("DUMP: %s", out);
        int r = send(client_fd, out, strlen(out), MSG_NOSIGNAL);
        if (r <= 0) {
          printf("Connection closed while dumping data");
          break;
        }
      }
    } else {
      printf("ADMIN auth failed\n");
      send_line_fd(client_fd, "BADTOK\n");
    }

  } else if (strcasecmp(cmd, "PING") == 0) {
    send_line_fd(client_fd, "PONG\n");

  } else {
    send_line_fd(client_fd, "UNK\n");
  }
}

/*
 * Start the telemetry server on given port, optionally preloading monitors from
 * monitors_dat file.
 */
int start_server(const char *monitors_dat, uint16_t port) {
  // generate admin token
  read_admin_token(admin_token);
  printf("Admin token: %s\n", admin_token);
  // preload monitors.dat if provided
  if (monitors_dat) {
    FILE *f = fopen(monitors_dat, "r");

    time_t now = time(NULL);
    srand((unsigned int)(now ^ getpid()));
    if (f) {
      char line[256];
      while (fgets(line, sizeof(line), f)) {
        size_t L = strlen(line);
        if (L && line[L - 1] == '\n')
          line[L - 1] = '\0';
        if (find_host_by_name(line) == -1) {
          char tok[TOKEN_SZ];
          gen_hex_random(tok, 12);
          add_host(line, tok);
          monitors[mon_count - 1]->cpu = (rand() % 10000) / 100.0;
          monitors[mon_count - 1]->ram = (rand() % 10000) / 100.0;
        }
      }
      fclose(f);
    } else {
      perror("fopen monitors.dat");
      return -1;
    }
  }

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    return -1;
  }
  int opt = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  sa.sin_port = htons(port);
  if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
    perror("bind");
    close(s);
    return -1;
  }
  if (listen(s, 64) != 0) {
    perror("listen");
    close(s);
    return -1;
  }

  fprintf(stderr, "PWNlemetry server listening on %d\n", port);

  struct pollfd pfds[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; ++i)
    pfds[i].fd = -1;
  pfds[0].fd = s;
  pfds[0].events = POLLIN;

  while (1) {
    int nfds = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i)
      if (pfds[i].fd != -1)
        nfds = i + 1;
    int ret = poll(pfds, nfds ? nfds : 1, 10000);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      perror("poll");
      break;
    }
    if (ret == 0)
      continue;

    // new connection?
    if (pfds[0].revents & POLLIN) {
      int cfd = accept(s, NULL, NULL);
      if (cfd >= 0) {
        int slot = -1;
        for (int i = 1; i < MAX_CLIENTS; ++i)
          if (pfds[i].fd == -1) {
            slot = i;
            break;
          }
        if (slot == -1) {
          close(cfd);
        } else {
          pfds[slot].fd = cfd;
          pfds[slot].events = POLLIN;
          fprintf(stderr, "[+] client connected slot=%d\n", slot);
        }
      }
    }

    // handle clients (process one command per readiness event to avoid
    // blocking)
    for (int i = 1; i < MAX_CLIENTS; ++i) {
      if (pfds[i].fd == -1)
        continue;
      int fd = pfds[i].fd;
      if (pfds[i].revents & (POLLIN)) {
        char line[MAX_LINE];
        ssize_t rl = readline_fd(fd, line, sizeof(line));
        if (rl <= 0) {
          close(fd);
          fprintf(stderr, "[-] client fd=%d disconnected\n", fd);
          pfds[i].fd = -1;
          continue;
        }
        // process exactly one command and return to poll loop
        process_command(fd, line);
      }
      if (pfds[i].revents & (POLLHUP | POLLERR)) {
        close(fd);
        pfds[i].fd = -1;
      }
    }
  }

  close(s);
  return 0;
}
