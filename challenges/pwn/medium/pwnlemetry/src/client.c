#include "client.h"
#include "server.h"
#include "utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct client_cfg {
  char server[128];
  uint16_t port;
  char token[128];
  char hostname[128];
  bool running;
  bool connected;
  pthread_mutex_t lock;
  pthread_t tid;
};

static struct client_cfg cfg;

/*
 * Print help message for client shell
 */
static void print_help(void) {
  printf("pwnlemetry client shell commands:\n");
  printf("  set server <ip>      - set server IP (default 127.0.0.1)\n");
  printf("  set port <num>       - set server port (default 31337)\n");
  printf("  set token <tok>      - manually set token\n");
  printf("  set hostname <host>  - manually set token\n");
  printf("  show                 - display current config\n");
  printf("  register             - register this host with server (request "
         "token)\n");
  printf("  start                - start telemetry (background)\n");
  printf("  stop                 - stop telemetry\n");
  printf("  view                 - view current monitored host data\n");
  printf("  dump data <tok>      - dump all monitor data (requries admin "
         "token)\n");
  printf("  help                 - show this help\n");
  printf("  quit                 - exit client\n");
}

/*
 * Initialize client configuration
 */
static void init_client_cfg(void) {
  pthread_mutex_init(&cfg.lock, NULL);
  strncpy(cfg.server, "127.0.0.1", sizeof(cfg.server) - 1);
  cfg.port = DEFAULT_PORT;
  strncpy(cfg.hostname, "localhost", sizeof(cfg.hostname) - 1);
  cfg.token[0] = '\0';
  cfg.running = false;
  cfg.connected = false;
}

/*
 * Telemetry thread function
 */
static void *telemetry_thread(void *arg) {
  char server[128];
  uint16_t port = 0;
  char token[128];
  char hostname[128];

  int sock = -1;

  while (1) {
    pthread_mutex_lock(&cfg.lock);
    bool run = cfg.running;
    char new_server[128];
    strncpy(new_server, cfg.server, sizeof(new_server));
    uint16_t new_port = cfg.port;
    char new_token[128];
    strncpy(new_token, cfg.token, sizeof(new_token));
    char new_hostname[128];
    strncpy(new_hostname, cfg.hostname, sizeof(new_hostname));
    pthread_mutex_unlock(&cfg.lock);

    if (!run) {
      if (sock >= 0) {
        close(sock);
      }
      break;
    }

    // on first iteration or if config changed, (re)connect
    if (strncmp(token, new_token, sizeof(token)) != 0 ||
        strncmp(server, new_server, sizeof(server)) != 0 || port != new_port ||
        strncmp(hostname, new_hostname, sizeof(hostname)) || sock < 0) {
      strncpy(server, new_server, sizeof(server));
      server[sizeof(server) - 1] = '\0';
      port = new_port;
      strncpy(token, new_token, sizeof(token));
      token[sizeof(token) - 1] = '\0';
      strncpy(hostname, new_hostname, sizeof(hostname));
      hostname[sizeof(hostname) - 1] = '\0';
      if (sock >= 0) {
        close(sock);
      }

      sock = connect_to(server, port);
      if (sock >= 0) {
        fprintf(stderr, "[telemetry] reconnected to %s:%d\n", server, port);
      } else {
        fprintf(stderr, "[telemetry] cannot connect to %s:%d\n", server, port);
        pthread_mutex_lock(&cfg.lock);
        cfg.running = false;
        pthread_mutex_unlock(&cfg.lock);
        return NULL;
      }
    }

    // send an update
    double cpu = (rand() % 10000) / 100.0;
    double ram = (rand() % 10000) / 100.0;
    char msg[512];
    snprintf(msg, sizeof(msg), "UPDATE %s %s %.2f %.2f\n", hostname, token, cpu,
             ram);

    int res = send_line_fd(sock, msg);
    if (res <= 0) {
      fprintf(stderr, "[telemetry] send failed, reconnecting\n");
      close(sock);
      sock = -1;
      sleep(5);
      continue;
    }

    // try to read small response (non-blocking short)
    char buf[256];
    ssize_t r = recv(sock, buf, sizeof(buf) - 1, 0);
    if (r > 0) {
      buf[r] = '\0';
      fprintf(stderr, "[telemetry] server: %s", buf);
    }

    // sleep and loop
    for (int i = 0; i < 5; ++i) {
      pthread_mutex_lock(&cfg.lock);
      bool run2 = cfg.running;
      pthread_mutex_unlock(&cfg.lock);
      if (!run2) {
        close(sock);
        break;
      }
      sleep(1);
    }
  }
  return NULL;
}

/*
 * Client shell main loop
 */
static void client_shell(void) {
  char line[512];
  print_help();
  while (1) {
    printf("pwnlemetry> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin))
      break;
    // trim newline
    size_t L = strlen(line);
    if (L && line[L - 1] == '\n')
      line[L - 1] = '\0';
    if (strlen(line) == 0)
      continue;

    char *tok = strtok(line, " ");
    if (!tok)
      continue;

    if (strcasecmp(tok, "set") == 0) {
      char *what = strtok(NULL, " ");
      if (!what) {
        printf("usage: set server|port|token <value>\n");
        continue;
      }
      if (strcasecmp(what, "server") == 0) {
        char *val = strtok(NULL, " ");
        if (!val) {
          printf("usage: set server <ip>\n");
          continue;
        }
        pthread_mutex_lock(&cfg.lock);
        strncpy(cfg.server, val, sizeof(cfg.server) - 1);
        cfg.server[sizeof(cfg.server) - 1] = '\0';
        pthread_mutex_unlock(&cfg.lock);
        printf("server set to %s\n", cfg.server);
      } else if (strcasecmp(what, "port") == 0) {
        char *val = strtok(NULL, " ");
        if (!val) {
          printf("usage: set port <num>\n");
          continue;
        }
        int p = atoi(val);
        if (p <= 0 || p > 65535) {
          printf("invalid port\n");
          continue;
        }
        pthread_mutex_lock(&cfg.lock);
        cfg.port = (uint16_t)p;
        pthread_mutex_unlock(&cfg.lock);
        printf("port set to %d\n", p);
      } else if (strcasecmp(what, "token") == 0) {
        char *val = strtok(NULL, " ");
        if (!val) {
          printf("usage: set token <token>\n");
          continue;
        }
        pthread_mutex_lock(&cfg.lock);
        strncpy(cfg.token, val, sizeof(cfg.token) - 1);
        cfg.token[sizeof(cfg.token) - 1] = '\0';
        pthread_mutex_unlock(&cfg.lock);
        printf("token set\n");
      } else if (strcasecmp(what, "hostname") == 0) {
        char *val = strtok(NULL, " ");
        if (!val) {
          printf("usage: set hostname <hostname>\n");
          continue;
        }
        pthread_mutex_lock(&cfg.lock);
        char *old_hostname = strdup(cfg.hostname);
        strncpy(cfg.hostname, val, sizeof(cfg.hostname) - 1);
        cfg.hostname[sizeof(cfg.hostname) - 1] = '\0';
        char *server = cfg.server;
        uint16_t port = cfg.port;
        bool running = cfg.running;
        pthread_mutex_unlock(&cfg.lock);
        printf("hostname set to %s\n", cfg.hostname);

        if (running && server[0] != '\0' && port != 0 && cfg.token[0] != '\0') {
          printf("telemetry is running, edit in upstream server\n");
          int sock = connect_to(server, port);
          if (sock < 0) {
            printf("cannot connect to %s:%d\n", server, port);
            continue;
          }
          char msg[512];
          snprintf(msg, sizeof(msg), "EDIT %s %s %zu\n", old_hostname,
                   cfg.token, strlen(cfg.hostname));
          int res = send_line_fd(sock, msg);
          if (res > 0) {
            send_line_fd(sock, cfg.hostname);
            char *resp = recv_line_timeout(sock, 1000);
            if (resp) {
              printf("server: %s", resp);
              free(resp);
            } else {
              printf("no response\n");
            }
          } else {
            printf("send failed\n");
          }
        }
        free(old_hostname);

      } else {
        printf("unknown set target\n");
      }

    } else if (strcasecmp(tok, "show") == 0) {
      pthread_mutex_lock(&cfg.lock);
      printf("server: %s\n", cfg.server);
      printf("port:   %d\n", cfg.port);
      printf("hostname: %s\n", cfg.hostname);
      printf("token:  %s\n", cfg.token[0] ? cfg.token : "(not set)");
      printf("telemetry: %s\n", cfg.running ? "running" : "stopped");
      pthread_mutex_unlock(&cfg.lock);

    } else if (strcasecmp(tok, "register") == 0) {
      pthread_mutex_lock(&cfg.lock);
      char server[128];
      strncpy(server, cfg.server, sizeof(server));
      uint16_t port = cfg.port;
      char hostname[128];
      strncpy(hostname, cfg.hostname, sizeof(hostname));
      pthread_mutex_unlock(&cfg.lock);

      int sock = connect_to(server, port);
      if (sock < 0) {
        printf("cannot connect to %s:%d\n", server, port);
        continue;
      }
      char header[128];
      int hlen = (int)strlen(hostname);
      int n = snprintf(header, sizeof(header), "REGISTER %d\n", hlen);
      send(sock, header, n, 0);
      send(sock, hostname, hlen, 0);
      char *resp = recv_line_timeout(sock, 1000);
      if (!resp) {
        printf("no response\n");
        close(sock);
        continue;
      }
      if (strncmp(resp, "TOKEN ", 6) == 0) {
        char *t = resp + 6;
        char *nl = strchr(t, '\n');
        if (nl)
          *nl = '\0';
        pthread_mutex_lock(&cfg.lock);
        strncpy(cfg.token, t, sizeof(cfg.token) - 1);
        cfg.token[sizeof(cfg.token) - 1] = '\0';
        pthread_mutex_unlock(&cfg.lock);
        printf("Registered. Received token: %s\n", cfg.token);
      } else {
        printf("Register response: %s\n", resp);
      }
      free(resp);
      close(sock);

    } else if (strcasecmp(tok, "start") == 0) {
      pthread_mutex_lock(&cfg.lock);
      if (cfg.running) {
        printf("telemetry already running\n");
        pthread_mutex_unlock(&cfg.lock);
        continue;
      }
      cfg.running = true;
      if (pthread_create(&cfg.tid, NULL, telemetry_thread, NULL) != 0) {
        cfg.running = false;
        printf("failed to start telemetry thread\n");
        pthread_mutex_unlock(&cfg.lock);
        continue;
      }
      pthread_mutex_unlock(&cfg.lock);
      printf("telemetry started\n");

    } else if (strcasecmp(tok, "stop") == 0) {
      pthread_mutex_lock(&cfg.lock);
      if (!cfg.running) {
        printf("telemetry not running\n");
        pthread_mutex_unlock(&cfg.lock);
        continue;
      }
      cfg.running = false;
      pthread_mutex_unlock(&cfg.lock);
      pthread_join(cfg.tid, NULL);
      printf("telemetry stopped\n");

    } else if (strcasecmp(tok, "dump") == 0) {
      pthread_mutex_lock(&cfg.lock);
      char server[128];
      strncpy(server, cfg.server, sizeof(server));
      uint16_t port = cfg.port;
      pthread_mutex_unlock(&cfg.lock);

      char *command = strtok(NULL, " ");
      if (!command || strcasecmp(command, "data") != 0) {
        printf("usage: dump data <token>\n");
        continue;
      }
      char *token = strtok(NULL, " ");
      if (!token) {
        printf("usage: dump data <token>\n");
        continue;
      }

      int sock = connect_to(server, port);
      if (sock < 0) {
        printf("cannot connect to %s:%d\n", server, port);
        continue;
      }

      char header[128];
      snprintf(header, sizeof(header), "DUMP %s\n", token);
      send_line_fd(sock, header);

      char *resp = recv_line_timeout(sock, 1000);
      if (resp) {
        printf("server:\n%s", resp);
        free(resp);
      }

      close(sock);
    } else if (strcasecmp(tok, "view") == 0) {
      pthread_mutex_lock(&cfg.lock);
      char server[128];
      strncpy(server, cfg.server, sizeof(server));
      uint16_t port = cfg.port;
      char token[128];
      strncpy(token, cfg.token, sizeof(token));
      char hostname[128];
      strncpy(hostname, cfg.hostname, sizeof(hostname));
      pthread_mutex_unlock(&cfg.lock);

      if (token[0] == '\0' || hostname[0] == '\0') {
        printf("set hostname and token first\n");
        continue;
      }

      int sock = connect_to(server, port);
      if (sock < 0) {
        printf("cannot connect to %s:%d\n", server, port);
        continue;
      }
      char msg[512];
      snprintf(msg, sizeof(msg), "VIEW %s %s\n", hostname, token);
      send_line_fd(sock, msg);
      char *resp = recv_line_timeout(sock, 1000);
      if (resp) {
        printf("server: %s", resp);
        free(resp);
      } else {
        printf("no response\n");
      }
      close(sock);

    } else if (strcasecmp(tok, "help") == 0) {
      print_help();

    } else if (strcasecmp(tok, "quit") == 0 || strcasecmp(tok, "exit") == 0) {
      pthread_mutex_lock(&cfg.lock);
      bool was_running = cfg.running;
      if (cfg.running)
        cfg.running = false;
      pthread_mutex_unlock(&cfg.lock);
      if (was_running)
        pthread_join(cfg.tid, NULL);
      printf("bye\n");
      break;

    } else {
      printf("unknown command, type 'help'\n");
    }
  }
}

/*
 * Start client task
 */
int start_client(uint16_t port_override) {
  init_client_cfg();
  if (port_override != 0)
    cfg.port = port_override;
  client_shell();
  return 0;
}
