/*
 * net/tcp.c — TCP 协议 (Day 48: 完整三次握手)
 *
 * 对照: reference/linux-7.1/net/ipv4/tcp.c, tcp_input.c, tcp_output.c
 */

#include "kernel.h"

/* TCP 状态 */
enum { TCP_CLOSED, TCP_SYN_SENT, TCP_ESTABLISHED, TCP_LAST_ACK };

/* TCP 首部 */
struct tcp_hdr {
    unsigned short src_port, dst_port;
    unsigned int   seq, ack;
    unsigned char  data_off;
    unsigned char  flags;
    unsigned short window;
    unsigned short checksum, urgent;
} __attribute__((packed));

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

static unsigned short htons(unsigned short v) { return (v>>8)|(v<<8); }
static unsigned int   htonl(unsigned int v)   { return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v>>8)&0xFF00)|((v>>24)&0xFF); }

/* 地址 */
static unsigned char my_ip[4]  = {10,0,2,15};
static unsigned char peer_ip[4]= {10,0,2,2};

/* TCP 状态 */
static int tcp_state = TCP_CLOSED;
static unsigned int tcp_iss = 0x12345678;
static unsigned int tcp_snd_nxt, tcp_rcv_nxt, tcp_irs;

/* TCP 校验和 (伪首部 + TCP首部 + 数据) */
static unsigned short tcp_checksum(struct tcp_hdr *tcp, int dlen)
{
    struct { unsigned int src, dst; unsigned char zero, proto; unsigned short len; } ph;
    ph.src = *(unsigned int*)my_ip; ph.dst = *(unsigned int*)peer_ip;
    ph.zero=0; ph.proto=6; ph.len=htons(20+dlen);

    unsigned int sum = 0;
    unsigned short *p = (unsigned short*)&ph;
    for (int i=0; i<6; i++) sum += p[i];
    p = (unsigned short*)tcp;
    for (int i=0; i<(20+dlen)/2; i++) sum += p[i];
    if ((20+dlen)&1) sum += ((unsigned char*)tcp)[20+dlen-1] << 8;
    while (sum>>16) sum = (sum&0xFFFF)+(sum>>16);
    return (unsigned short)~sum;
}

/* 构建 TCP 包并通过 net_send_ip 发送 */
static void tcp_send_raw(int flags, unsigned int seq, unsigned int ack, void *data, int dlen)
{
    char buf[1500];
    struct tcp_hdr *tcp = (struct tcp_hdr *)buf;

    tcp->src_port = htons(12345);
    tcp->dst_port = htons(80);
    tcp->seq  = htonl(seq);
    tcp->ack  = htonl(ack);
    tcp->data_off = (5 << 4);
    tcp->flags = flags;
    tcp->window = htons(65535);
    tcp->checksum = 0;
    tcp->urgent = 0;

    if (data && dlen > 0)
        for (int i=0; i<dlen; i++) buf[20+i] = ((char*)data)[i];

    tcp->checksum = htons(tcp_checksum(tcp, dlen));

    extern void net_send_ip(unsigned char proto, unsigned char *dst,
                            void *data, int len);
    net_send_ip(6, peer_ip, buf, 20 + dlen);
}

void tcp_connect(void)
{
    if (tcp_state != TCP_CLOSED) return;
    tcp_snd_nxt = tcp_iss;
    tcp_send_raw(TCP_SYN, tcp_iss, 0, 0, 0);
    tcp_snd_nxt++;
    tcp_state = TCP_SYN_SENT;
    serial_puts("TCP: SYN sent\r\n");
}

void tcp_input(void *packet, int len)
{
    if (len < 54) return; /* ETH+IP+TCP */
    struct tcp_hdr *tcp = (struct tcp_hdr *)((char*)packet + 34);
    unsigned int seq = htonl(tcp->seq), ack = htonl(tcp->ack);

    switch (tcp_state) {
    case TCP_SYN_SENT:
        if ((tcp->flags & (TCP_SYN|TCP_ACK)) == (TCP_SYN|TCP_ACK)) {
            tcp_irs = seq; tcp_rcv_nxt = seq + 1;
            tcp_send_raw(TCP_ACK, tcp_snd_nxt, tcp_rcv_nxt, 0, 0);
            tcp_state = TCP_ESTABLISHED;
            serial_puts("TCP: established\r\n");
            /* 通知 HTTP 层 */
            extern void http_server_start(void);
            http_server_start();
        }
        break;
    case TCP_ESTABLISHED:
        if (tcp->flags & TCP_FIN) {
            tcp_rcv_nxt = seq + 1;
            tcp_send_raw(TCP_ACK|TCP_FIN, tcp_snd_nxt, tcp_rcv_nxt, 0, 0);
            tcp_state = TCP_LAST_ACK;
        } else {
            int hdr_len = (tcp->data_off >> 4) * 4;
            int dlen = len - 54 - (hdr_len - 20);
            if (dlen > 0) {
                char *data = (char*)packet + 34 + hdr_len;
                serial_puts("TCP: rx ");
                print_hex64(dlen);
                serial_puts(" bytes\r\n");
                /* 路由到 HTTP */
                extern void http_handle(void *, int);
                http_handle(data, dlen);
            }
        }
        break;
    case TCP_LAST_ACK:
        if (tcp->flags & TCP_ACK) { tcp_state = TCP_CLOSED; serial_puts("TCP: closed\r\n"); }
        break;
    }
}

/* tcp_send_data — ESTABLISHED 状态下发送数据 */
void tcp_send_data(void *data, int len)
{
    if (tcp_state != TCP_ESTABLISHED) return;
    tcp_send_raw(TCP_ACK|TCP_PSH, tcp_snd_nxt, tcp_rcv_nxt, data, len);
    tcp_snd_nxt += len;
    serial_puts("TCP: sent ");
    print_hex64(len);
    serial_puts(" bytes\r\n");
}
