/*
 * net/http.c — HTTP/1.1 服务器 (Day 49)
 *
 * 对照: RFC 7230 (用户态协议, 无内核对照)
 */

#include "kernel.h"

/* ---- 辅助: 简化 sprintf ---- */
static int sprintf_stub(char *buf, const char *fmt, ...)
{
    unsigned long *args = (unsigned long*)(&fmt+1);
    char *start = buf;
    while (*fmt) {
        if (*fmt=='%' && fmt[1]=='d') {
            int v=(int)*args++; fmt+=2;
            if (v==0) { *buf++='0'; continue; }
            char t[16]; int n=0;
            while (v) { t[n++]='0'+v%10; v/=10; }
            while (n) *buf++ = t[--n];
        } else if (*fmt=='%' && fmt[1]=='s') {
            const char *s=(const char*)*args++; fmt+=2;
            while (*s) *buf++ = *s++;
        } else { *buf++ = *fmt++; }
    }
    *buf = 0;
    return (int)(buf - start);
}

static int http_ready = 0;

void http_server_start(void)
{
    http_ready = 1;
    serial_puts("HTTP: server ready\r\n");
}

void http_handle(void *data, int len)
{
    if (!http_ready || len < 4) return;
    char *req = (char *)data;

    if (req[0]!='G' || req[1]!='E' || req[2]!='T') return;

    const char *body;
    if (req[4]=='/' && (req[5]==' '||req[5]=='\r')) {
        body = "<html><body><h1>MyKernel HTTP Server</h1>"
               "<p>TCP/IP stack working!</p></body></html>";
    } else {
        body = "<html><body><h1>404</h1></body></html>";
    }

    char rsp[1024]; char *r = rsp;
    r += sprintf_stub(r, "HTTP/1.1 200 OK\r\nServer: MyKernel\r\nContent-Type: text/html\r\n\r\n");
    for (const char *s = body; *s; s++) *r++ = *s;
    int total = r - rsp;

    extern void tcp_send_data(void *, int);
    tcp_send_data(rsp, total);
}
