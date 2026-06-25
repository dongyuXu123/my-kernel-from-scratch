/*
 * net/tcp.c — TCP 协议
 *
 * 对照: reference/linux-7.1/net/ipv4/tcp.c (protocol)
 *       reference/linux-7.1/net/ipv4/tcp_output.c (output)
 */
#include "kernel.h"

/* TCP 状态 */
#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_SENT 2
#define TCP_SYN_RCVD 3
#define TCP_ESTABLISHED 4

/* TCP 头 */
struct tcp_hdr {
    unsigned short src_port, dst_port;
    unsigned int   seq, ack;
    unsigned char  off;       /* data offset << 4 */
    unsigned char  flags;     /* FIN=1,SYN=2,RST=4,PSH=8,ACK=16,URG=32 */
    unsigned short win;
    unsigned short cksum;
    unsigned short urp;
} __attribute__((packed));

static unsigned short tcp_sport = 12345;
static unsigned int   tcp_seq = 0x12345678;
static int tcp_state = TCP_CLOSED;

/* IP 地址 */
static unsigned char my_ip[4] = {10,0,2,15};
static unsigned char gw_ip[4] = {10,0,2,2};

/* 发送 TCP 段 */
static unsigned short cksum16(void *d, int len) {
    unsigned int s=0; unsigned short *p=d;
    for(int i=0;i<len/2;i++) s+=p[i];
    if(len&1) s+=((unsigned char*)d)[len-1]<<8;
    while(s>>16) s=(s&0xFFFF)+(s>>16);
    return (unsigned short)~s;
}

void tcp_send_syn(unsigned int dst_ip, unsigned short dst_port) {
    serial_puts("TCP: sending SYN...\r\n");
    tcp_state = TCP_SYN_SENT;
    tcp_seq = 0x12345678;
    
    /* 构建 TCP SYN 段 (简化: 直接发送, 无 IP 封装 — 实际需要 IP 层) */
    struct tcp_hdr tcp;
    tcp.src_port = 12345; tcp.dst_port = dst_port;
    tcp.seq = tcp_seq; tcp.ack = 0;
    tcp.off = 5 << 4; tcp.flags = 0x02; /* SYN */
    tcp.win = 4096; tcp.cksum = 0; tcp.urp = 0;
    tcp.cksum = cksum16(&tcp, 20);
    
    extern int e1000_send_packet(void *data, int len);
    char pkt[64];
    /* Ethernet + IP header simplified */
    for(int i=0;i<14;i++) pkt[i]=0; /* placeholder */
    for(int i=0;i<20;i++) pkt[14+i]=((char*)&tcp)[i];
    e1000_send_packet(pkt, 34);
    serial_puts("TCP: SYN sent\r\n");
}

void tcp_init(void) {
    serial_puts("TCP: initialized\r\n");
    tcp_state = TCP_CLOSED;
}
