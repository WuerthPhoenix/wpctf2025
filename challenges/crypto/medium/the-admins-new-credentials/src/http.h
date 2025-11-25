#pragma once

#include <openssl/ssl.h>

typedef int fd_t;

typedef enum { None, Admin } request_role_t;

// Simple HTTP request structure
typedef struct {
    int method;
    request_role_t role;
    char *url;
    char *body;
} http_request_t;

http_request_t *http_request_read(SSL *ssl);
void http_request_free(http_request_t *req);

// Simple HTTP response structure
typedef struct {
    char *response;
    size_t length;
    size_t capacity;
} http_response_t;

http_response_t http_response_create(int status_code, const char *status_message);
void http_response_header(http_response_t *res, const char *header);
void http_response_header_value(http_response_t *res, const char *header, const char *value);
void http_response_body(http_response_t *res, const char *body);
void http_response_file_body(http_response_t *res, fd_t file);
http_response_t http_response_serve_file_with_code(const char *path, const char *content_type,
                                                   int status_code, const char *status_message);
#define http_response_serve_file(path, content_type)                                               \
    http_response_serve_file_with_code(path, content_type, 200, "OK")
#define http_response_serve_html(path) http_response_serve_file(path, "application/html")
#define http_response_serve_error(status_code, status_message)                                     \
    http_response_serve_file_with_code("templates/" #status_code ".html", "application/html",      \
                                       status_code, status_message);

void http_response_send(http_response_t res, SSL *ssl);
