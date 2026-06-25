/*
 * net/http.c — HTTP/1.1 服务器 (Day 49)
 *
 * 对照: (无直接对照 — Linux 内核无内置 HTTP 协议栈)
 *       参考 RFC 7230 (HTTP/1.1 Message Syntax and Routing)
 */

#include "kernel.h"

/* HTTP 状态 */
static int http_listening = 0;
static char http_response[512];

/* serial_sprintf — 简化版 sprintf (仅 %d 和 %s) */
static int serial_sprintf(char *buf, const char *fmt, ...)
{
    unsigned long *args = (unsigned long *)(&fmt + 1);
    char *start = buf;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd') {
                int val = (int)args[0]; args++;
                if (val == 0) { *buf++ = '0'; }
                else {
                    char tmp[16]; int n = 0;
                    if (val < 0) { *buf++ = '-'; val = -val; }
                    while (val) { tmp[n++] = '0' + (val % 10); val /= 10; }
                    while (n) *buf++ = tmp[--n];
                }
            } else if (*fmt == 's') {
                const char *s = (const char *)args[0]; args++;
                while (*s) *buf++ = *s++;
            }
        } else {
            *buf++ = *fmt;
        }
        fmt++;
    }
    *buf = 0;
    return (int)(buf - start);
}

/* http_server_start — 开始监听 HTTP 请求 */
void http_server_start(void)
{
    http_listening = 1;
    serial_puts("HTTP: server listening on port 80\r\n");
}

/* http_handle_request — 解析 HTTP 请求并返回响应 */
int http_handle_request(const char *request, int len, char *response, int maxlen)
{
    if (!http_listening) return 0;
    if (len < 4) return 0;

    /* 简化解析: 只处理 GET /path HTTP/1.x */
    if (request[0] != 'G' || request[1] != 'E' || request[2] != 'T')
        return 0;

    /* 找路径 */
    const char *path = request + 4;  /* 跳过 "GET " */
    const char *end = path;
    while (*end && *end != ' ' && *end != '\r' && *end != '\n') end++;
    int path_len = (int)(end - path);

    serial_puts("HTTP: GET ");
    for (int i = 0; i < path_len; i++) serial_putchar(path[i]);
    serial_puts("\r\n");

    /* 路由 */
    const char *body;
    int body_len;
    int status = 200;
    const char *status_text = "OK";

    if (path_len == 1 && path[0] == '/') {
        body = "<html><body><h1>Welcome to MyKernel HTTP Server!</h1>"
               "<p>Day 49: HTTP/1.1 Server</p>"
               "<p><a href='/about'>About</a></p></body></html>";
        body_len = 125;
    } else if (path_len == 6 && path[0] == '/' && path[1] == 'a') {
        body = "<html><body><h1>About</h1>"
               "<p>This kernel was built from scratch!</p>"
               "<p><a href='/'>Home</a></p></body></html>";
        body_len = 115;
    } else {
        body = "<html><body><h1>404 Not Found</h1></body></html>";
        body_len = 48;
        status = 404;
        status_text = "Not Found";
    }

    /* 构建响应 */
    char *r = response;
    /* 状态行 */
    r += serial_sprintf(r, "HTTP/1.1 %d %s\r\n", status, status_text);
    r += serial_sprintf(r, "Server: MyKernel/1.0\r\n");
    r += serial_sprintf(r, "Content-Type: text/html; charset=utf-8\r\n");
    r += serial_sprintf(r, "Content-Length: %d\r\n", body_len);
    r += serial_sprintf(r, "Connection: close\r\n");
    r += serial_sprintf(r, "\r\n");
    /* Body */
    for (int i = 0; i < body_len && (r - response) < maxlen - 1; i++)
        *r++ = body[i];
    *r = 0;

    return (int)(r - response);
}
