// Session handling for wpftpd
#ifndef WPFTPD_WP_FTP_SESSION_H
#define WPFTPD_WP_FTP_SESSION_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "wp_ftp_command.h"

// Node for queued raw command strings (to be parsed by worker thread)
typedef struct ftp_cmd_str_queue_node {
    char *cmd_str;
    struct ftp_cmd_str_queue_node *next;
} ftp_cmd_str_queue_node;

typedef struct ftp_session {
    int ctrl_fd;
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in client_addr;
    char data_host[INET_ADDRSTRLEN];
    int data_port;
    int has_port;
    char line_buf[CTRL_READ_BUF];
    char username[64];
    int have_user;
    int authed;
    ftp_cmd_node *active_cmds; // async transfer tracking
    // Command string queue processed by worker thread
    ftp_cmd_str_queue_node *q_head;
    ftp_cmd_str_queue_node *q_tail;
    pthread_mutex_t q_mutex;
    pthread_cond_t q_cond;
    int q_terminate;
    pthread_t worker_tid;
    int worker_started;
} ftp_session;

// Handle a single FTP control connection (forked child calls this)
void process_session(int cfd, struct sockaddr_in *caddr);

#endif // WPFTPD_WP_FTP_SESSION_H
