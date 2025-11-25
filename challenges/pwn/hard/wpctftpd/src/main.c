#include <stdio.h>
#include "include/conf.h"
#include <strings.h> // for strcasecmp (may be unused after refactor)
#include "include/wp_ftp_session.h"

#define CONTROL_PORT 21

// Session handling moved to wp_ftp_session.c

static int create_listen_socket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 16) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-p <port>]\n", prog);
}

int main(int argc, char **argv) {
    int port = CONTROL_PORT; // default
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-p requires a port number\n");
                return 1;
            }
            char *end = NULL;
            long v = strtol(argv[++i], &end, 10);
            if (!end || *end || v <= 0 || v > 65535) {
                fprintf(stderr, "Invalid port: %s\n", argv[i]);
                return 1;
            }
            port = (int) v;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    init_pool();

    signal(SIGCHLD, SIG_IGN); // avoid zombies

    int listen_fd = create_listen_socket((uint16_t) port);
    if (listen_fd < 0) {
        perror("bind/listen");
        if (port == 21) fprintf(stderr, "(Hint: need root or cap_net_bind_service for port 21)\n");
        return 1;
    }
    printf("wpctftpd listening on port %d\n", port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cfd = accept(listen_fd, (struct sockaddr *) &caddr, &clen);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            else break;
        }
        pid_t pid = fork();
        if (pid < 0) {
            close(cfd);
            continue;
        }
        if (pid == 0) {
            // child
            close(listen_fd);
            process_session(cfd, &caddr);
            destroy_pool();
            _exit(0); // terminate child after single session
        } else {
            // parent
            close(cfd);
        }
    }

    close(listen_fd);
    destroy_pool();
    return 0;
}
