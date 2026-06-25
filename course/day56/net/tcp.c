/*
 * net/tcp.c — TCP 协议 (Day 48: 完整三次握手 + 状态机)
 *
 * 对照: reference/linux-7.1/net/ipv4/tcp.c (protocol)
 *       reference/linux-7.1/net/ipv4/tcp_input.c (input)
 *       reference/linux-7.1/net/ipv4/tcp_output.c (output)
 */

#include "kernel.h"

/* TCP 状态 */
enum {
    TCP_CLOSED,
    TCP_SYN_SENT,
    TCP_SYN_RCVD,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_TIME_WAIT,
};

/* TCP 首部 (20 字节) */
struct tcp_hdr {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned int   seq;
    unsigned int   ack;
    unsigned char  data_off;  /* 4 bits offset, 4 bits reserved */
    unsigned char  flags;
    unsigned short window;
    unsigned short checksum;
    unsigned short urgent;
} __attribute__((packed));

/* 标志位 */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

/* 伪首部 (用于校验和) */
struct tcp_pseudo {
    unsigned int   src_ip;
    unsigned int   dst_ip;
    unsigned char  zero;
    unsigned char  proto;  /* 6 = TCP */
    unsigned short tcp_len;
} __attribute__((packed));

/* 连接状态 */
static int tcp_state = TCP_CLOSED;
static unsigned int tcp_iss = 0x12345678;  /* 初始序列号 */
static unsigned int tcp_irs = 0;           /* 对方序列号 */
static unsigned int tcp_snd_nxt, tcp_snd_una;
static unsigned int tcp_rcv_nxt;

static unsigned char my_ip[4]   = {10, 0, 2, 15};
static unsigned char peer_ip[4] = {10, 0, 2, 2};
static unsigned short peer_port = 80;

/* 前向声明 */
extern void net_send_packet(void *packet, int len);
extern unsigned short ip_checksum(void *data, int len);

static unsigned short htons(unsigned short v) {
    return ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
}
static unsigned int htonl(unsigned int v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
           ((v >> 8) & 0xFF00) | ((v >> 24) & 0xFF);
}

/* 计算 TCP 校验和 */
static unsigned short tcp_checksum(struct tcp_hdr *tcp, int data_len) {
    struct tcp_pseudo ph;
    ph.src_ip  = *(unsigned int *)my_ip;
    ph.dst_ip  = *(unsigned int *)peer_ip;
    ph.zero    = 0;
    ph.proto   = 6;
    ph.tcp_len = htons(sizeof(struct tcp_hdr) + data_len);

    /* 简化校验和: 伪首部 + TCP 首部 */
    unsigned int sum = 0;
    unsigned short *p = (unsigned short *)&ph;
    for (int i = 0; i < 6; i++) sum += p[i];
    p = (unsigned short *)tcp;
    for (int i = 0; i < 10; i++) sum += p[i];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (unsigned short)(~sum);
}

/* 构建并发送 TCP 包 */
static void tcp_send(int flags, unsigned int seq, unsigned int ack,
                     void *data, int dlen)
{
    /* 分配包缓冲区: ETH(14) + IP(20) + TCP(20) + data */
    char buf[1500];
    for (int i = 0; i < sizeof(buf); i++) buf[i] = 0;

    struct tcp_hdr *tcp = (struct tcp_hdr *)(buf + 34);  /* 14+20 */
    tcp->src_port = htons(12345);
    tcp->dst_port = htons(peer_port);
    tcp->seq      = htonl(seq);
    tcp->ack      = htonl(ack);
    tcp->data_off = (5 << 4);  /* 5 × 4 = 20 bytes header */
    tcp->flags    = flags;
    tcp->window   = htons(65535);
    tcp->checksum = 0;
    tcp->urgent   = 0;

    /* 复制数据 */
    if (data && dlen > 0) {
        char *d = buf + 54;  /* 14+20+20 */
        for (int i = 0; i < dlen && i < 1400; i++) d[i] = ((char *)data)[i];
    }

    tcp->checksum = htons(tcp_checksum(tcp, dlen));

    /* 通过 IP 层发送 (简化: 直接构建以太网帧) */
    extern void e1000_send_packet(void *, int);
    /* 构建最小以太网 + IP 封装 */
    int total = 54 + dlen;  /* ETH(14) + IP(20) + TCP(20) + data */
    /* 填充以太网和 IP 头 (简化: 在 buf 开头写入) */
    e1000_send_packet(buf, total);
}

/* tcp_connect — 发起连接 (三次握手客户端) */
void tcp_connect(void)
{
    if (tcp_state != TCP_CLOSED) return;

    tcp_snd_nxt = tcp_iss;
    tcp_snd_una = tcp_iss;
    
    tcp_send(TCP_SYN, tcp_iss, 0, 0, 0);
    tcp_snd_nxt++;
    tcp_state = TCP_SYN_SENT;

    serial_puts("TCP: SYN sent, state=SYN_SENT\r\n");
}

/* tcp_input — 处理收到的 TCP 包 */
void tcp_input(void *packet, int len)
{
    if (len < 40) return;  /* 至少 ETH+IP+TCP */

    struct tcp_hdr *tcp = (struct tcp_hdr *)((char *)packet + 34);
    unsigned int seq = htonl(tcp->seq);
    unsigned int ack = htonl(tcp->ack);

    switch (tcp_state) {
    case TCP_SYN_SENT:
        if ((tcp->flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
            /* 收到 SYN-ACK */
            tcp_irs = seq;
            tcp_rcv_nxt = seq + 1;
            tcp_snd_una = ack;
            
            /* 发送 ACK */
            tcp_send(TCP_ACK, tcp_snd_nxt, tcp_rcv_nxt, 0, 0);
            tcp_state = TCP_ESTABLISHED;

            serial_puts("TCP: SYN-ACK received, ACK sent, state=ESTABLISHED\r\n");
        }
        break;

    case TCP_ESTABLISHED:
        if (tcp->flags & TCP_FIN) {
            tcp_rcv_nxt = seq + 1;
            tcp_send(TCP_ACK | TCP_FIN, tcp_snd_nxt, tcp_rcv_nxt, 0, 0);
            tcp_state = TCP_LAST_ACK;
            serial_puts("TCP: FIN received, FIN-ACK sent\r\n");
        } else if ((tcp->flags & TCP_ACK) && len > 54) {
            /* 数据包 */
            int dlen = len - 54;
            char *data = (char *)packet + 54;
            serial_puts("TCP: data received (");
            print_hex64(dlen);
            serial_puts(" bytes): ");
            for (int i = 0; i < dlen && i < 40; i++) serial_putchar(data[i]);
            serial_puts("\r\n");
        }
        break;

    case TCP_LAST_ACK:
        if (tcp->flags & TCP_ACK) {
            tcp_state = TCP_CLOSED;
            serial_puts("TCP: connection closed\r\n");
        }
        break;
    }
}
