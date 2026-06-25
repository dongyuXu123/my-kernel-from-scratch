/*
 * net/http.c — HTTP 客户端
 *
 * 对照: (无直接对照 — Linux 内核无内置 HTTP 协议栈，参考 RFC 7230)
 */
#include "kernel.h"

void http_get(const char *url) {
    serial_puts("HTTP: GET "); serial_puts(url); serial_puts("\r\n");
    
    /* 构建 HTTP/1.0 GET 请求 */
    const char *req = "GET / HTTP/1.0\r\nHost: 10.0.2.2\r\n\r\n";
    
    /* 发送 (使用 e1000 发包 — 简化演示) */
    extern int e1000_send_packet(void *data, int len);
    char pkt[128];
    for (int i = 0; req[i]; i++) pkt[i] = req[i];
    int len = 0; while (req[len]) len++;
    e1000_send_packet(pkt, len);
    
    serial_puts("HTTP: request sent\r\n");
}
