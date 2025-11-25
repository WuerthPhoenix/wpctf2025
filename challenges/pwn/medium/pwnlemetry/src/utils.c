#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Connect to host:port, return socket fd or -1 on error
 */
int connect_to(const char *host, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  if (inet_pton(AF_INET, host, &sa.sin_addr) <= 0) {
    close(sock);
    return -1;
  }
  sa.sin_port = htons(port);
  if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
    close(sock);
    return -1;
  }
  return sock;
}

/*
 * Read a single line with a small timeout from sock (returns malloc'd buffer
 * or NULL)
 */
char *recv_line_timeout(int sock, int timeout_ms) {
  struct pollfd pfd;
  pfd.fd = sock;
  pfd.events = POLLIN;
  int ret = poll(&pfd, 1, timeout_ms);
  if (ret <= 0)
    return NULL;
  char buf[512];
  ssize_t r = recv(sock, buf, sizeof(buf) - 1, 0);
  if (r <= 0)
    return NULL;
  buf[r] = '\0';
  char *out = strdup(buf);
  return out;
}

/*
 * Safe send that avoids SIGPIPE on broken pipe
 * Returns number of bytes sent, or -1 on error
 */
static int safe_send_fd(int fd, const char *s, size_t len) {
  ssize_t sent = send(fd, s, len, MSG_NOSIGNAL);
  if (sent <= 0)
    return -1;
  return (int)sent;
}

/*
 * Send a line (string) to fd
 * Returns number of bytes sent, or -1 on error
 */
int send_line_fd(int fd, const char *s) {
  return safe_send_fd(fd, s, strlen(s));
}

/*
 * Read exactly len bytes from fd into buf. Returns number of bytes read, or -1
 * on error.
 */
ssize_t read_n_fd(int fd, void *buf, size_t len) {
  size_t got = 0;
  while (got < len) {
    ssize_t r = recv(fd, (char *)buf + got, len - got, 0);
    if (r <= 0)
      return -1;
    got += r;
  }
  return (ssize_t)got;
}

/*
 * Read a line (up to newline) from fd. Returns length (>0), 0 on EOF, -1 on
 * error. The newline is replaced with null-terminator.
 */
ssize_t readline_fd(int fd, char *buf, size_t bufsize) {
  size_t pos = 0;
  while (pos + 1 < bufsize) {
    char c;
    ssize_t r = recv(fd, &c, 1, 0);
    if (r == 0)
      return 0; // closed
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    buf[pos++] = c;
    if (c == '\n') {
      buf[pos - 1] = '\0';
      break;
    }
  }
  return (ssize_t)pos;
}

/*
 * Generate len bytes of random data and output as hex string into outbuf
 * (outbuf must be at least len*2+1 bytes). Max len is 32.
 */
void gen_hex_random(char *outbuf, size_t len) {
  if (len > 32)
    len = 32;
  uint8_t tmp[32] = {0};
  FILE *ur = fopen("/dev/urandom", "rb");
  if (ur) {
    int r = fread(tmp, 1, len, ur);
    if (r != (int)len) {
      fprintf(stderr, "fread /dev/urandom failed\n");
      exit(1);
    }
    fclose(ur);
  } else {
    srand(time(NULL) ^ getpid());
    for (size_t i = 0; i < len; ++i)
      tmp[i] = rand() & 0xff;
  }
  for (size_t i = 0; i < len; ++i) {
    sprintf(outbuf + i * 2, "%02x", tmp[i]);
  }
  outbuf[len * 2] = '\0';
}
