#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include "include/conf.h"
#include "include/wp_ftp_session.h"
#include "include/wp_ftp_command.h"
#include <strings.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <signal.h>

// Provide a non-static safe_write for reuse
ssize_t safe_write(int fd, const void *buf, size_t len) {
    const char *p = (const char *) buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, p + off, len - off, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t) n;
    }
    return (ssize_t) off;
}

static void format_mode(mode_t m, char *out) {
    out[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : '-';
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

static int open_data_connection_for(ftp_session *sess, ftp_cmd *cmd) {
    (void) sess; // currently unused
    if (!cmd || !cmd->has_port) return -1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t) cmd->data_port);
    // set a timeout of 5 seconds to the connect
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (inet_pton(AF_INET, cmd->data_host, &sa.sin_addr) != 1) {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        close(fd);
        return -1;
    }
    cmd->data_fd = fd;
    return fd;
}

void cmd_ls(ftp_session *sess, ftp_cmd *cmd, const char *path) {
    printf("executing LIST\n");
    if (!cmd->has_port) {
        safe_write(sess->ctrl_fd, "503 Bad sequence of commands. Use PORT first\r\n", 47);
        return;
    }
    const char *dir_path = (path && *path) ? path : ".";
    DIR *d = opendir(dir_path);
    if (!d) {
        safe_write(sess->ctrl_fd, "550 Failed to open directory\r\n", 31);
        return;
    }
    int data_fd = open_data_connection_for(sess, cmd);
    if (data_fd < 0) {
        closedir(d);
        safe_write(sess->ctrl_fd, "425 Can't open data connection\r\n", 34);
        return;
    }
    safe_write(sess->ctrl_fd, "150 Opening data connection for LIST\r\n", 38);
    struct dirent *de;
    char line[1024];
    char fullpath[PATH_MAX];
    time_t now = time(NULL);
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        if (snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, de->d_name) >= (int) sizeof(fullpath)) continue;
        struct stat st;
        if (lstat(fullpath, &st) < 0) continue;
        char perms[11];
        format_mode(st.st_mode, perms);
        struct passwd *pw = getpwuid(st.st_uid);
        const char *user = pw ? pw->pw_name : "?";
        struct group *gr = getgrgid(st.st_gid);
        const char *grp = gr ? gr->gr_name : "?";
        char tbuf[32];
        struct tm *mt = localtime(&st.st_mtime);
        if (mt) {
            double diff = difftime(now, st.st_mtime);
            if (diff < 0) diff = -diff;
            if (diff > 15552000) strftime(tbuf, sizeof(tbuf), "%b %e  %Y", mt);
            else strftime(tbuf, sizeof(tbuf), "%b %e %H:%M", mt);
        } else strcpy(tbuf, "??? ?? ??:??");
        ssize_t n;
        if (S_ISLNK(st.st_mode)) {
            char target[PATH_MAX];
            ssize_t rl = readlink(fullpath, target, sizeof(target) - 1);
            if (rl >= 0) target[rl] = '\0';
            else strcpy(target, "?");
            n = snprintf(line, sizeof(line), "%s %3lu %s %s %8lld %s %s -> %s\r\n", perms, (unsigned long) st.st_nlink,
                         user, grp, (long long) st.st_size, tbuf, de->d_name, target);
        } else {
            n = snprintf(line, sizeof(line), "%s %3lu %s %s %8lld %s %s\r\n", perms, (unsigned long) st.st_nlink, user,
                         grp, (long long) st.st_size, tbuf, de->d_name);
        }
        if (n > 0 && n < (ssize_t) sizeof(line)) safe_write(data_fd, line, (size_t) n);
    }
    closedir(d);
    close(data_fd);
    cmd->data_fd = -1;
    safe_write(sess->ctrl_fd, "226 Directory send OK\r\n", 24);
}

// Download thread
static void *download_thread(void *arg) {
    struct {
        ftp_session *sess;
        ftp_cmd *cmd;
        char path[PATH_MAX];
    } *ctx = arg;
    ftp_session *sess = ctx->sess;
    ftp_cmd *cmd = ctx->cmd;
    const char *path = ctx->path;
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        safe_write(sess->ctrl_fd, "550 File not found\r\n", 21);
        goto done;
        goto done;
    }
    int data_fd = open_data_connection_for(sess, cmd);
    if (data_fd < 0) {
        safe_write(sess->ctrl_fd, "425 Can't open data connection\r\n", 34);
        close(fd);
        goto done;
    }
    safe_write(sess->ctrl_fd, "150 Opening data connection for RETR\r\n", 38);
    char buf[4096];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (safe_write(data_fd, buf, (size_t) n) < 0) break;
    }
    close(fd);
    close(data_fd);
    cmd->data_fd = -1;
    safe_write(sess->ctrl_fd, "226 Transfer complete\r\n", 23);
done:
    remove_active_cmd(sess, cmd);
    wpfree(cmd);
    wpfree(ctx);
    return NULL;
}

// Upload thread
static void *upload_thread(void *arg) {
    struct {
        ftp_session *sess;
        ftp_cmd *cmd;
        char path[PATH_MAX];
    } *ctx = arg;
    ftp_session *sess = ctx->sess;
    ftp_cmd *cmd = ctx->cmd;
    const char *path = ctx->path;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        safe_write(sess->ctrl_fd, "550 Cannot create file\r\n", 24);
        goto done;
    }
    int data_fd = open_data_connection_for(sess, cmd);
    if (data_fd < 0) {
        safe_write(sess->ctrl_fd, "425 Can't open data connection\r\n", 34);
        close(fd);
        goto done;
    }
    safe_write(sess->ctrl_fd, "150 Opening data connection for STOR\r\n", 38);
    char buf[4096];
    for (;;) {
        ssize_t n = recv(data_fd, buf, sizeof(buf), 0);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        ssize_t w = write(fd, buf, (size_t) n);
        if (w != n) { break; }
    }
    close(fd);
    close(data_fd);
    cmd->data_fd = -1;
    safe_write(sess->ctrl_fd, "226 Transfer complete\r\n", 23);
done:
    printf("Upload of %s complete or failed\n", path);
    fflush(stdout);
    remove_active_cmd(sess, cmd);
    wpfree(cmd);

    wpfree(ctx);
    return NULL;
}

void cmd_download(ftp_session *sess, ftp_cmd *cmd, const char *path) {
    if (!cmd->has_port) {
        safe_write(sess->ctrl_fd, "503 Bad sequence of commands. Use PORT first\r\n", 47);
        return;
    }
    if (!path || !*path) {
        safe_write(sess->ctrl_fd, "501 Missing path\r\n", 18);
        return;
    }
    add_active_cmd(sess, cmd);
    struct {
        ftp_session *sess;
        ftp_cmd *cmd;
        char path[PATH_MAX];
    } *ctx = wpmalloc(sizeof(*ctx));
    if (!ctx) {
        safe_write(sess->ctrl_fd, "421 Memory allocation failed\r\n", 29);
        remove_active_cmd(sess, cmd);
        return;
    }
    ctx->sess = sess;
    ctx->cmd = cmd;
    strncpy(ctx->path, path, PATH_MAX - 1);
    ctx->path[PATH_MAX - 1] = '\0';
    pthread_t tid;
    pthread_create(&tid, NULL, download_thread, ctx);
    pthread_detach(tid);
    cmd->is_async = 1;
}

void cmd_upload(ftp_session *sess, ftp_cmd *cmd, const char *path) {
    printf("executing STOR\n");
    if (!cmd->has_port) {
        safe_write(sess->ctrl_fd, "503 Bad sequence of commands. Use PORT first\r\n", 47);
        return;
    }
    if (!path || !*path) {
        safe_write(sess->ctrl_fd, "501 Missing path\r\n", 18);
        return;
    }
    add_active_cmd(sess, cmd);
    cmd->is_async = 1;
    struct {
        ftp_session *sess;
        ftp_cmd * cmd;
        char path[PATH_MAX];
    } *ctx = wpmalloc(sizeof(*ctx));
    if (!ctx) {
        safe_write(sess->ctrl_fd, "421 Memory allocation failed\r\n", 29);
        remove_active_cmd(sess, cmd);
        return;
    }
    ctx->sess = sess;
    ctx->cmd = cmd;
    strncpy(ctx->path, path, PATH_MAX - 1);
    ctx->path[PATH_MAX - 1] = '\0';
    pthread_t tid;
    pthread_create(&tid, NULL, upload_thread, ctx);
    pthread_detach(tid);
}

void deinit_cmd_upload(ftp_cmd *cmd) {
    printf("cleaning upload cache");
}

void handle_port(ftp_session *sess, const char *arg) {
    if (!arg) {
        safe_write(sess->ctrl_fd, "501 Missing PORT args\r\n", 24);
        return;
    }
    char tmp[128];
    strncpy(tmp, arg, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(tmp, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        safe_write(sess->ctrl_fd, "501 Bad PORT format\r\n", 23);
        return;
    }
    if (h1 < 0 || h1 > 255 || h2 < 0 || h2 > 255 || h3 < 0 || h3 > 255 || h4 < 0 || h4 > 255 || p1 < 0 || p1 > 255 || p2
        < 0 || p2 > 255) {
        safe_write(sess->ctrl_fd, "501 Bad PORT values\r\n", 23);
        return;
    }
    uint32_t ip = (uint32_t) ((h1 << 24) | (h2 << 16) | (h3 << 8) | h4);
    struct in_addr ina;
    ina.s_addr = htonl(ip);
    if (!inet_ntop(AF_INET, &ina, sess->data_host, sizeof(sess->data_host))) {
        safe_write(sess->ctrl_fd, "501 IP conversion failed\r\n", 27);
        return;
    }
    sess->data_port = (p1 << 8) | p2;
    sess->has_port = 1;
    safe_write(sess->ctrl_fd, "200 PORT set\r\n", 14);
}

void cmd_pwd(ftp_session *sess) {
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        safe_write(sess->ctrl_fd, "550 PWD failed\r\n", 17);
        return;
    }
    char resp[PATH_MAX + 64];
    int n = snprintf(resp, sizeof(resp), "257 \"%s\" is current directory\r\n", cwd);
    if (n > 0) safe_write(sess->ctrl_fd, resp, (size_t) n);
}

void cmd_cwd(ftp_session *sess, const char *path) {
    if (!path || !*path) {
        safe_write(sess->ctrl_fd, "501 Missing path\r\n", 18);
        return;
    }
    if (chdir(path) == 0) safe_write(sess->ctrl_fd, "250 Directory changed\r\n", 23);
    else safe_write(sess->ctrl_fd, "550 Failed to change directory\r\n", 33);
}

void cmd_dele(ftp_session *sess, const char *path) {
    if (!path || !*path) {
        safe_write(sess->ctrl_fd, "501 Missing path\r\n", 18);
        return;
    }
    if (unlink(path) == 0) safe_write(sess->ctrl_fd, "250 File deleted\r\n", 19);
    else safe_write(sess->ctrl_fd, "550 Delete failed\r\n", 20);
}

void cmd_feat(ftp_session *sess) {
    safe_write(sess->ctrl_fd, "211-Features:\r\n", 15);
    static const char *features[] = {"PORT", "LIST", "RETR", "STOR", "PWD", "CWD", "DELE", "FEAT", NULL};
    for (int i = 0; features[i]; ++i) {
        char line[64];
        int n = snprintf(line, sizeof(line), " %s\r\n", features[i]);
        if (n > 0) safe_write(sess->ctrl_fd, line, (size_t) n);
    }
    safe_write(sess->ctrl_fd, "211 End\r\n", 9);
}

void identify_cmd(const char *cmd, ftp_cmd *out) {
    if (!cmd) out->type = FTP_CMD_UNKNOWN;
    else if (!strcasecmp(cmd, "USER")) out->type = FTP_CMD_USER;
    else if (!strcasecmp(cmd, "PASS")) out->type = FTP_CMD_PASS;
    else if (!strcasecmp(cmd, "FEAT")) out->type = FTP_CMD_FEAT;
    else if (!strcasecmp(cmd, "PORT")) out->type = FTP_CMD_PORT;
    else if (!strcasecmp(cmd, "LIST")) out->type = FTP_CMD_LIST;
    else if (!strcasecmp(cmd, "PWD")) out->type = FTP_CMD_PWD;
    else if (!strcasecmp(cmd, "CWD")) out->type = FTP_CMD_CWD;
    else if (!strcasecmp(cmd, "DELE")) out->type = FTP_CMD_DELE;
    else if (!strcasecmp(cmd, "RETR")) {
        out->type = FTP_CMD_RETR;
    }
    else if (!strcasecmp(cmd, "STOR")) {
        out->type = FTP_CMD_STOR;
        out->deinit_func = &deinit_cmd_upload;
    }
    else if (!strcasecmp(cmd, "QUIT") || !strcasecmp(cmd, "EXIT")) out->type = FTP_CMD_QUIT;
    else out->type = FTP_CMD_UNKNOWN;

}

void parse_command(ftp_session *sess, ftp_cmd *out, const char *line, ssize_t linelen) {
    memset(out, 0, sizeof(*out));
    out->type = FTP_CMD_UNKNOWN;
    out->data_fd = -1;
    out->raw = (char *) wpmalloc(linelen + 1);
    if (!out->raw) return;
    memcpy(out->raw, line, linelen);
    out->raw[linelen] = '\0'; // correct termination
    char *p = out->raw;
    while (*p == ' ' || *p == '\t') ++p;
    if (*p == '\0') {
        out->arg = NULL;
        return;
    }
    char *arg = strchr(p, ' ');
    if (arg) {
        *arg++ = '\0';
        while (*arg == ' ' || *arg == '\t') ++arg;
        if (!*arg) arg = NULL;
    }
    out->arg = arg;
    identify_cmd(p, out);
    if (sess && sess->has_port) {
        size_t hlen = strnlen(sess->data_host, sizeof(out->data_host) - 1);
        memcpy(out->data_host, sess->data_host, hlen);
        out->data_host[hlen] = '\0';
        out->data_port = sess->data_port;
        out->has_port = 1;
    } else out->has_port = 0;
}

void add_active_cmd(ftp_session *sess, ftp_cmd *cmd) {
    ftp_cmd_node *node = (ftp_cmd_node *) wpmalloc(sizeof(ftp_cmd_node));
    if (!node) return;
    node->cmd = cmd;
    node->next = sess->active_cmds;
    sess->active_cmds = node;
}

void remove_active_cmd(ftp_session *sess, ftp_cmd *cmd) {
    ftp_cmd_node **pp = &sess->active_cmds;
    while (*pp) {
        if ((*pp)->cmd == cmd) {
            ftp_cmd_node *tmp = *pp;
            *pp = (*pp)->next;
            if (tmp->cmd->deinit_func)
                tmp->cmd->deinit_func(cmd);
            wpfree(tmp);
            break;
        }
        pp = &(*pp)->next;
    }
}

int is_active_cmd(ftp_session *sess, ftp_cmd *cmd) {
    for (ftp_cmd_node *n = sess->active_cmds; n; n = n->next) if (n->cmd == cmd) return 1;
    return 0;
}