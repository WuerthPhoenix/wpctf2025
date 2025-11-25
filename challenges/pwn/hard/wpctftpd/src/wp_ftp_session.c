#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include "include/conf.h"
#include "include/wp_ftp_session.h"
#include "include/wp_ftp_command.h"
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

// Extern from command implementation
extern ssize_t safe_write(int fd, const void *buf, size_t len);

static void cleanup_session(ftp_session *sess);

// Session termination globals
static volatile sig_atomic_t g_session_terminate = 0;
static ftp_session *g_current_session = NULL;

static void sig_session_term(int signo) {
    // Async-signal-safe minimal message distinguishing signal source
    if (signo == SIGUSR1) {
        const char msg[] = "[wpctftpd] control connection peer closed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    } else if (signo == SIGPIPE) {
        const char msg[] = "[wpctftpd] SIGPIPE (write on closed socket)\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    } else {
        const char msg[] = "[wpctftpd] session termination signal\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }
    cleanup_session(g_current_session);
}

// Receive a CRLF line (LF terminator) into buffer
static ssize_t recv_line(int fd, char *buf, size_t max) {
    size_t off = 0;
    while (off + 1 < max) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) {
            raise(SIGUSR1);
            return 0;
        }
        if (n < 0) {
            if (errno == EINTR) {
                // if (g_session_terminate) return -1;
                continue;
            }
            if (errno == ECONNRESET || errno == EPIPE) { raise(SIGUSR1); }
            return -1;
        }
        if (c == '\r') continue;
        if (c == '\n') break;
        buf[off++] = c;
    }
    buf[off] = '\0';
    return (ssize_t) off;
}

static int verify_credentials(const char *user, const char *pass) {
    if (!user || !*user || !pass) return 0;
    struct passwd *pw = getpwnam(user);
    if (!pw) return 0;
    const char *stored = pw->pw_passwd;
    if (!stored) return 0;
#ifdef __linux__
    if (stored[0] == 'x' && stored[1] == '\0') {
        struct spwd *sp = getspnam(user);
        if (sp && sp->sp_pwdp) stored = sp->sp_pwdp;
        else return 0;
    }
#endif
    if (stored[0] == '!' || stored[0] == '*') return 0;
    if (stored[0] == '\0') return pass[0] == '\0';
    char *calc = crypt(pass, stored);
    if (!calc) return 0;
    return strcmp(calc, stored) == 0;
}

// Queue helpers for string queue
// Remove local typedef of ftp_cmd_str_queue_node (now in header)

static void enqueue_cmd_str(ftp_session *sess, const char *cmd_str, size_t len) {
    if (!sess || !cmd_str) return;
    ftp_cmd_str_queue_node *node = (ftp_cmd_str_queue_node *) wpmalloc(sizeof(*node));
    if (!node) return;
    char *copy = (char *) wpmalloc(len + 1);
    if (!copy) { wpfree(node); return; }
    memcpy(copy, cmd_str, len);
    copy[len] = '\0';
    node->cmd_str = copy;
    node->next = NULL;
    pthread_mutex_lock(&sess->q_mutex);
    if (sess->q_tail) sess->q_tail->next = node; else sess->q_head = node;
    sess->q_tail = node;
    pthread_cond_signal(&sess->q_cond);
    pthread_mutex_unlock(&sess->q_mutex);
}

static char *dequeue_cmd_str(ftp_session *sess) {
    pthread_mutex_lock(&sess->q_mutex);
    while (!sess->q_head) {
        pthread_cond_wait(&sess->q_cond, &sess->q_mutex);
    }
    if (!sess->q_head) {
        pthread_mutex_unlock(&sess->q_mutex);
        return NULL;
    }
    ftp_cmd_str_queue_node *node = sess->q_head;
    sess->q_head = node->next;
    if (!sess->q_head) sess->q_tail = NULL;
    pthread_mutex_unlock(&sess->q_mutex);
    char *cmd_str = node->cmd_str;
    wpfree(node);
    return cmd_str;
}

static bool get_cmd_str_queue_size(ftp_session *sess) {
    pthread_mutex_lock(&sess->q_mutex);
    int size = 0;
    for (ftp_cmd_str_queue_node *n = sess->q_head; n; n = n->next) size++;
    pthread_mutex_unlock(&sess->q_mutex);
    return size;
}

static void process_command(ftp_session *sess, ftp_cmd *cmd) {
    int cfd = sess->ctrl_fd;
    switch (cmd->type) {
        case FTP_CMD_QUIT:
            safe_write(cfd, "221 Bye\r\n", 9);
            g_session_terminate = 1;
            // signal termination to main loop
            pthread_mutex_lock(&sess->q_mutex);
            sess->q_terminate = 1;
            pthread_cond_broadcast(&sess->q_cond);
            pthread_mutex_unlock(&sess->q_mutex);
            break;
        case FTP_CMD_USER:
            if (!cmd->arg) safe_write(cfd, "501 Missing username\r\n", 22);
            else {
                memset(sess->username, 0, sizeof(sess->username));
                strncpy(sess->username, cmd->arg, sizeof(sess->username) - 1);
                sess->have_user = 1;
                sess->authed = 0;
                safe_write(cfd, "331 User name okay, need password\r\n", 36);
            }
            break;
        case FTP_CMD_PASS:
            if (!sess->have_user) safe_write(cfd, "503 Bad sequence of commands\r\n", 32);
            else if (!cmd->arg) safe_write(cfd, "501 Missing password\r\n", 22);
            else {
                int allow_any = getenv("WPCTFTPD_NO_AUTH") != NULL;
                if (allow_any || verify_credentials(sess->username, cmd->arg)) {
                    sess->authed = 1;
                    safe_write(cfd, "230 User logged in\r\n", 21);
                } else {
                    sess->authed = 0;
                    safe_write(cfd, "530 Login incorrect\r\n", 21);
                }
            }
            break;
        case FTP_CMD_FEAT:
            cmd_feat(sess);
            break;
        default:
            if (!sess->authed) {
                safe_write(cfd, "530 Not logged in\r\n", 19);
                break;
            }
            switch (cmd->type) {
                case FTP_CMD_PORT: handle_port(sess, cmd->arg); break;
                case FTP_CMD_LIST: cmd_ls(sess, cmd, cmd->arg); break;
                case FTP_CMD_PWD: cmd_pwd(sess); break;
                case FTP_CMD_CWD: cmd_cwd(sess, cmd->arg); break;
                case FTP_CMD_DELE: cmd_dele(sess, cmd->arg); break;
                case FTP_CMD_RETR: cmd_download(sess, cmd, cmd->arg); break;
                case FTP_CMD_STOR: cmd_upload(sess, cmd, cmd->arg); break;
                case FTP_CMD_UNKNOWN: safe_write(cfd, "502 Command not implemented\r\n", 29); break;
                default: break;
            }
            break;
    }
}

void free_ftp_cmd(ftp_cmd *cmd) {
    if (cmd->deinit_func) cmd->deinit_func(cmd);
    if (cmd->raw) wpfree(cmd->raw);
    wpfree(cmd);
}

static void *cmd_worker(void *arg) {
    ftp_session *sess = (ftp_session *) arg;
    for (;;) {
        char *cmd_str = dequeue_cmd_str(sess);
        if (!cmd_str) {
            sleep(1);
            continue; // termination
        }
        ftp_cmd *cmd = (ftp_cmd *) wpmalloc(sizeof(ftp_cmd));
        if (!cmd) {
            wpfree(cmd_str);
            continue;
        }
        memset(cmd, 0, sizeof(*cmd));
        parse_command(sess, cmd, cmd_str, strlen(cmd_str));
        wpfree(cmd_str);
        process_command(sess, cmd);
        if (!cmd->is_async || !is_active_cmd(sess, cmd)) {
            free_ftp_cmd(cmd);
        }
        if (get_cmd_str_queue_size(sess) == 0 && g_session_terminate) break;
    }
    return NULL;
}

void cleanup_cmd_node(ftp_cmd_node *cmd) {
    if (cmd->cmd->deinit_func) cmd->cmd->deinit_func(cmd->cmd);
    wpfree(cmd->cmd->arg);
    wpfree(cmd);
}

static void cleanup_session(ftp_session *sess) {
    wpfreepool();
    printf("cleanup_session\n");
    // if (!sess) return;
    // // Free any remaining queued command strings
    // while (sess->q_head) {
    //     ftp_cmd_str_queue_node *node = sess->q_head;
    //     //sess->q_head = node->next;
    //     if (node && node->cmd_str) wpfree(node->cmd_str);
    //     wpfree(node);
    // }
    // // Free all active commands
    // for (ftp_cmd_node * cmd = sess->active_cmds; cmd != NULL; cmd = cmd->next) {
    //     cleanup_cmd_node(cmd);
    // }
    // // wpfree(sess); // Uncomment if session is dynamically managed

}


void process_session(int cfd, struct sockaddr_in *caddr) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_session_term;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    ftp_session *sess = (ftp_session *) wpmalloc(sizeof(ftp_session));
    if (!sess) {
        const char *msg = "421 Memory allocation failed\r\n";
        safe_write(cfd, msg, strlen(msg));
        close(cfd);
        return;
    }

    memset(sess, 0, sizeof(*sess));
    sess->ctrl_fd = cfd;
    sess->client_addr = *caddr;
    inet_ntop(AF_INET, &caddr->sin_addr, sess->client_ip, sizeof(sess->client_ip));
    pthread_mutex_init(&sess->q_mutex, NULL);
    pthread_cond_init(&sess->q_cond, NULL);
    sess->q_head = sess->q_tail = NULL;
    sess->q_terminate = 0;
    sess->worker_started = 0;
    g_current_session = sess;
    g_session_terminate = 0;

    // Start worker thread
    if (pthread_create(&sess->worker_tid, NULL, cmd_worker, sess) == 0) {
        sess->worker_started = 1;
    } else {
        safe_write(cfd, "421 Cannot start worker thread\r\n", 32);
        cleanup_session(sess);
        g_current_session = NULL;
        return;
    }

    const char *greet = "220 wpctftpd ready\r\n";
    safe_write(cfd, greet, strlen(greet));
    // while (!g_session_terminate || true) {
    while (true) {
        ssize_t nread = recv_line(cfd, sess->line_buf, sizeof(sess->line_buf));
        if (nread <= 0) break; // connection closed or error
        enqueue_cmd_str(sess, sess->line_buf, nread);
        // sleep for 100 ms
        // usleep(10000);
        // if (g_session_terminate) break;
    }
    // signal termination to worker
    pthread_mutex_lock(&sess->q_mutex);
    sess->q_terminate = 1;
    pthread_cond_broadcast(&sess->q_cond);
    pthread_mutex_unlock(&sess->q_mutex);

    // wait for child processes to finish
    while (sess->active_cmds) {
        sleep(1);
    }

    cleanup_session(sess);
    g_current_session = NULL;
}
