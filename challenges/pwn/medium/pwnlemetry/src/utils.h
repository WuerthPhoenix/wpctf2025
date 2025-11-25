#pragma once

#define MAX_MON 64
#define NAME_SZ 64
#define MAX_VIEW_LEN 128
#define TOKEN_SZ 32
#define ADMIN_TOKEN_SZ 64
#define MAX_LINE 1024
#define MAX_CLIENTS 48

#include <stdint.h>
#include <stdlib.h>

int connect_to(const char *, uint16_t);
char *recv_line_timeout(int, int);
int send_line_fd(int, const char *);
ssize_t readline_fd(int, char *, size_t);
void gen_hex_random(char *, size_t);
ssize_t read_n_fd(int, void *, size_t);
