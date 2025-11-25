#include "http.h"

#include <errno.h>
#include <fcntl.h>
#include <llhttp.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "log.h"

void http_response_append_string(http_response_t *res, const char *str) {
    int needed = strlen(str);
    while (res->capacity < res->length + needed) {
        res->capacity *= 2;
        res->response = realloc(res->response, res->capacity);
    }
    strcpy(res->response + res->length, str);
    res->length += needed;
}

http_response_t http_response_create(int status_code, const char *status_message) {
    http_response_t res = {0};
    res.capacity = 8192;
    res.response = malloc(res.capacity);
    int len =
        snprintf(res.response, res.capacity, "HTTP/1.0 %d %s\r\n", status_code, status_message);
    res.length = len;

    return res;
}

void http_response_header(http_response_t *res, const char *header) {
    http_response_append_string(res, header);
    http_response_append_string(res, "\r\n");
}

void http_response_header_value(http_response_t *res, const char *header, const char *value) {
    http_response_append_string(res, header);
    http_response_append_string(res, ": ");
    http_response_append_string(res, value);
    http_response_append_string(res, "\r\n");
}

void http_response_body(http_response_t *res, const char *body) {
    char content_length[64] = {0};
    snprintf(content_length, sizeof(content_length), "%zu", strlen(body));
    http_response_header_value(res, "Content-Length", content_length);
    http_response_append_string(res, "\r\n");
    http_response_append_string(res, body);
}

void http_response_send_file(http_response_t *res, fd_t file) {
    // Get file size
    off_t file_size = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);
    if (file_size < 0) {
        log_error("Failed to get file size");
        return;
    }

    char *body = malloc(file_size + 1);
    body[file_size] = '\0';
    read(file, body, file_size);

    http_response_body(res, body);
    free(body);
}

http_response_t http_response_serve_file_with_code(const char *path, const char *content_type,
                                                   int status_code, const char *status_message) {
    fd_t file = open(path, O_RDONLY);
    if (file < 0) {
        log_error("Failed to open file %s. Errno %d: %s", path, errno, strerror(errno));
        http_response_t res = http_response_create(500, "Internal Server Error");
        http_response_header(&res, "Content-Type: application/html");
        http_response_body(&res, "<h1>500: Internal Server Error</h1><p>Could not read file.</p>");
        return res;
    }

    http_response_t res = http_response_create(status_code, status_message);
    http_response_header_value(&res, "Content-Type", content_type);
    http_response_send_file(&res, file);

    close(file);

    return res;
}

void http_response_send(http_response_t res, SSL *ssl) {
    if (strcmp(res.response + res.length - 4, "\r\n\r\n") != 0) {
        http_response_append_string(&res, "\r\n");
    }
    log_debug("RESPONSE:%s", res.response);

    SSL_write(ssl, res.response, res.length);
    free(res.response);
}

typedef struct {
    http_request_t req;
    size_t body_length;
} http_request_context_t;

int on_url(llhttp_t *parser, const char *at, size_t length) {
    char *url = malloc(length + 1);
    memcpy(url, at, length);
    url[length] = '\0';
    log_debug("url: %s", url);
    http_request_t *context = (http_request_t *)parser->data;
    context->url = url;

    return 0;
}

int set_body_length(llhttp_t *parser, const char *at, size_t length) {
    http_request_context_t *context = (http_request_context_t *)parser->data;
    char *error = NULL;
    context->body_length = strtol(at, &error, 10);
    if (error != at + length) {
        log_error("Invalid Content-Length header: %.*s", length, at);
        return HPE_INVALID_CONTENT_LENGTH;
    }

    return 0;
}

int read_authorization_header(llhttp_t *parser, const char *at, size_t length) {
    http_request_context_t *context = (http_request_context_t *)parser->data;

    context->req.role = None;
    if (length > 7 && strncmp(at, "Bearer ", 7) == 0) {
        char *ADMIN_TOKEN = getenv("ADMIN_TOKEN");
        if (strncmp(at + 7, ADMIN_TOKEN, length - 7) == 0) {
            context->req.role = Admin;
        } else {
            log_debug("Invalid token provided: %.*s", (int)(length - 7), at + 7);
        }
    } else {
        log_warn("Unsupported Authorization header: %.*s", (int)length, at);
    }

    log_debug("Authorization: %d", context->req.role);
    return 0;
}

int on_header_field(llhttp_t *parser, const char *at, size_t length) {
    log_debug("Parsing header %.*s", length, at);
    llhttp_settings_t *settings = parser->settings;
    settings->on_header_value = NULL;
    if (strncasecmp("Content-Length", at, length) == 0) {
        settings->on_header_value = set_body_length;
    } else if (strncasecmp("Authorization:", at, length) == 0) {
        settings->on_header_value = read_authorization_header;
    }
    return 0;
}

typedef struct {
    char *buffer;
    bool authorization;
    size_t length;
    size_t capacity;
    size_t header_length;
} http_request_buffer_t;

http_request_buffer_t http_read_headers(SSL *ssl) {
    log_trace("Starting to read http headers");
    http_request_buffer_t buf = {
        .capacity = 8192, .length = 0, .buffer = malloc(8192), .header_length = 0};
    buf.buffer[0] = '\0';

    char *end_of_headers = NULL;
    while (strstr(buf.buffer, "\r\n\r\n") == NULL) {
        if (buf.length == buf.capacity - 1) {
            log_trace("Reallocating header buffer from %zu to %zu", buf.capacity, buf.capacity * 2);
            buf.capacity *= 2;
            buf.buffer = realloc(buf.buffer, buf.capacity);
        }

        log_trace("Reading more data from socket.");
        int bytes = SSL_read(ssl, buf.buffer + buf.length, buf.capacity - buf.length - 1);
        buf.length += bytes;
        buf.buffer[buf.length] = '\0';
        if (bytes == 0) {
            log_warn("Connection closed by peer.");
            free(buf.buffer);
            buf.buffer = NULL;
            return buf;
        }
        end_of_headers = strstr(buf.buffer, "\r\n\r\n");
    }
    buf.header_length = end_of_headers - buf.buffer;
    log_trace("Finished reading headers. Header length: %d, Prefetched body bytes: %d",
              (int)(buf.header_length), (int)(buf.length - buf.header_length - 4));
    log_debug("REQUEST HEADERS:%.*s", (int)(end_of_headers - buf.buffer), buf.buffer);
    return buf;
}

http_request_context_t *parse_http_headers(char *buffer, size_t length) {
    // initialize settings
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;

    // initialize parser
    llhttp_t parser = {0};
    llhttp_init(&parser, HTTP_REQUEST, &settings);
    http_request_context_t *context = malloc(sizeof(http_request_context_t));
    memset(context, 0, sizeof(http_request_context_t));
    parser.data = (void *)context;

    enum llhttp_errno err = llhttp_execute(&parser, buffer, length);
    if (err != HPE_OK) {
        log_error("Parse error: %s %s", llhttp_errno_name(err), llhttp_get_error_reason(&parser));
        http_request_free((http_request_t *)context);
        return NULL;
    }

    return context;
}

char *http_request_read_body(SSL *ssl, char *prefetched, size_t body_length) {
    char *body = malloc(body_length + 1);
    body[body_length] = '\0';
    strncpy(body, prefetched, body_length);
    size_t body_received = strlen(body);

    while (body_received < body_length) {
        log_trace("Reading http body! Received data: %d/%d", (int)body_received, (int)body_length);
        int bytes = SSL_read(ssl, body + body_received, body_length - body_received);
        if (bytes <= 0) {
            log_error("Connection closed while reading body");
            free(body);
            return NULL;
        }
        body_received += bytes;
    }
    return body;
}

http_request_t *http_request_read(SSL *ssl) {
    http_request_buffer_t headers = http_read_headers(ssl);
    if (headers.buffer == NULL) {
        return NULL;
    }

    http_request_context_t *context = parse_http_headers(headers.buffer, headers.header_length);
    if (context == NULL) {
        free(headers.buffer);
        return NULL;
    }

    if (context->body_length > 0) {
        char *prefetched_body = headers.buffer + headers.header_length + 4;
        context->req.body = http_request_read_body(ssl, prefetched_body, context->body_length);
        if (context->req.body == NULL) {
            http_request_free((http_request_t *)context);
            free(headers.buffer);
            return NULL;
        }
    }

    free(headers.buffer);

    // This is save, because the first field of the struct is the http_request_t.
    return (http_request_t *)context;
}

void http_request_free(http_request_t *req) {
    log_trace("Freeing http request data structures.");
    if (req->url)
        free(req->url);
    if (req->body)
        free(req->body);
    free(req);
}
