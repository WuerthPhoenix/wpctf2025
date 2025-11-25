#ifndef WPFTPD_WP_FTP_COMMAND_H
#define WPFTPD_WP_FTP_COMMAND_H

#include <netinet/in.h>
#include <sys/types.h>

struct ftp_session; // opaque to command layer for callers

// Command type enum exposed
typedef enum {
    FTP_CMD_USER,
    FTP_CMD_PASS,
    FTP_CMD_FEAT,
    FTP_CMD_PORT,
    FTP_CMD_LIST,
    FTP_CMD_PWD,
    FTP_CMD_CWD,
    FTP_CMD_DELE,
    FTP_CMD_RETR,
    FTP_CMD_STOR,
    FTP_CMD_QUIT,
    FTP_CMD_UNKNOWN
} ftp_cmd_type;

#define CTRL_READ_BUF 4096

// ftp command structure (allocated via wpmalloc)
typedef struct ftp_cmd {
    ftp_cmd_type type;
    void (*deinit_func)(struct ftp_cmd *self);
    char *raw;        // full raw line (mutable for parsing)
    char *arg;        // argument pointer inside raw or separate alloc
    char data_host[INET_ADDRSTRLEN];
    int data_port;
    int has_port;
    int data_fd;
    int is_async; // handled by background thread
} ftp_cmd;

// Linked list node for active async commands
typedef struct ftp_cmd_node {
    ftp_cmd *cmd;
    struct ftp_cmd_node *next;
} ftp_cmd_node;

void identify_cmd(const char *cmd, ftp_cmd *out);
void parse_command(struct ftp_session *sess, ftp_cmd *out, const char *line, ssize_t linelen);

// Active command list management
void add_active_cmd(struct ftp_session *sess, ftp_cmd *cmd);
void remove_active_cmd(struct ftp_session *sess, ftp_cmd *cmd);
int is_active_cmd(struct ftp_session *sess, ftp_cmd *cmd);

// Individual command handlers (control + data interaction)
void handle_port(struct ftp_session *sess, const char *arg);
void cmd_ls(struct ftp_session *sess, ftp_cmd *cmd, const char *path);
void cmd_download(struct ftp_session *sess, ftp_cmd *cmd, const char *path);
void cmd_upload(struct ftp_session *sess, ftp_cmd *cmd, const char *path);
void cmd_pwd(struct ftp_session *sess);
void cmd_cwd(struct ftp_session *sess, const char *path);
void cmd_dele(struct ftp_session *sess, const char *path);
void cmd_feat(struct ftp_session *sess);

#endif // WPFTPD_WP_FTP_COMMAND_H
