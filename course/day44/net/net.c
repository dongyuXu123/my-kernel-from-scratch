/*
 * net/net.c — 网络协议栈 (ARP + ICMP + TCP 路由)
 *
 * 对照: reference/linux-7.1/net/ipv4/arp.c, net/ipv4/icmp.c, net/ipv4/tcp_ipv4.c
 */

#include "kernel.h"

/* 我们的地址 */
static unsigned char my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static unsigned char my_ip[4]  = {10, 0, 2, 15};
static unsigned char gw_ip[4]  = {10, 0, 2, 2};
static unsigned char gw_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x57}; /* 默认，实际需 ARP */

/* e1000 接口 */
extern int e1000_send_packet(void *data, int len);

/* ================================================================
 * 工具函数
 * ================================================================ */
static unsigned short swap16(unsigned short x) { return (x >> 8) | (x << 8); }
unsigned short ip_checksum(void *data, int len)
{
    unsigned int sum = 0;
    unsigned short *p = (unsigned short *)data;
    for (int i = 0; i < len / 2; i++) sum += p[i];
    if (len & 1) sum += ((unsigned char *)data)[len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (unsigned short)~sum;
}

/* ================================================================
 * 以太网帧
 * ================================================================ */
struct eth_hdr {
    unsigned char  dst[6], src[6];
    unsigned short ethertype;
} __attribute__((packed));
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IP  0x0800

/* ================================================================
 * IP 首部
 * ================================================================ */
struct ip_hdr {
    unsigned char  ver_ihl, dscp;
    unsigned short total_len, id, flags_frag, ttl;
    unsigned char  proto;
    unsigned short checksum;
    unsigned char  src[4], dst[4];
} __attribute__((packed));
#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6

/* ================================================================
 * ARP
 * ================================================================ */
struct arp_hdr {
    unsigned short htype, ptype;
    unsigned char  hlen, plen;
    unsigned short oper;
    unsigned char  sha[6], spa[4], tha[6], tpa[4];
} __attribute__((packed));
#define ARP_REQUEST 1
#define ARP_REPLY   2

static struct arp_hdr *get_arp(void *packet) {
    return (struct arp_hdr *)((struct eth_hdr *)packet + 1);
}

static void arp_reply(void *packet)
{
    struct eth_hdr *eth = (struct eth_hdr *)packet;
    struct arp_hdr *arp = get_arp(packet);
    if (swap16(arp->oper) != ARP_REQUEST) return;
    for (int i = 0; i < 4; i++)
        if (arp->tpa[i] != my_ip[i]) return;

    /* 构建 ARP 应答 */
    char reply[64];
    struct eth_hdr *reth = (struct eth_hdr *)reply;
    struct arp_hdr *rarp = (struct arp_hdr *)(reth + 1);

    for (int i = 0; i < 6; i++) reth->dst[i] = eth->src[i];
    for (int i = 0; i < 6; i++) reth->src[i] = my_mac[i];
    reth->ethertype = swap16(ETHERTYPE_ARP);

    rarp->htype = swap16(1); rarp->ptype = swap16(ETHERTYPE_IP);
    rarp->hlen = 6; rarp->plen = 4;
    rarp->oper = swap16(ARP_REPLY);
    for (int i = 0; i < 6; i++) rarp->sha[i] = my_mac[i];
    for (int i = 0; i < 4; i++) rarp->spa[i] = my_ip[i];
    for (int i = 0; i < 6; i++) rarp->tha[i] = arp->sha[i];
    for (int i = 0; i < 4; i++) rarp->tpa[i] = arp->spa[i];

    e1000_send_packet(reply, sizeof(struct eth_hdr) + sizeof(struct arp_hdr));
    serial_puts("ARP: reply sent\r\n");
}

/* ================================================================
 * ICMP (仅 Echo Reply)
 * ================================================================ */
struct icmp_hdr {
    unsigned char  type, code;
    unsigned short checksum, id, seq;
} __attribute__((packed));
#define ICMP_ECHO_REQ 8
#define ICMP_ECHO_REP 0

static void icmp_reply(void *packet)
{
    struct eth_hdr *eth = (struct eth_hdr *)packet;
    struct ip_hdr *ip = (struct ip_hdr *)(eth + 1);
    struct icmp_hdr *icmp = (struct icmp_hdr *)(ip + 1);

    if (ip->proto != IPPROTO_ICMP) return;
    if (icmp->type != ICMP_ECHO_REQ) return;
    for (int i = 0; i < 4; i++) if (ip->dst[i] != my_ip[i]) return;

    int ip_hdr_len = (ip->ver_ihl & 0x0F) * 4;
    int icmp_len = swap16(ip->total_len) - ip_hdr_len;

    char reply[2048];
    struct eth_hdr *reth = (struct eth_hdr *)reply;
    struct ip_hdr *rip = (struct ip_hdr *)(reth + 1);
    struct icmp_hdr *ricmp = (struct icmp_hdr *)(rip + 1);
    char *payload = (char *)(ricmp + 1);

    /* Ethernet */
    for (int i = 0; i < 6; i++) { reth->dst[i] = eth->src[i]; reth->src[i] = my_mac[i]; }
    reth->ethertype = swap16(ETHERTYPE_IP);

    /* IP */
    rip->ver_ihl = 0x45; rip->dscp = 0;
    rip->total_len = swap16(20 + 8 + icmp_len);
    rip->id = swap16(0); rip->flags_frag = 0; rip->ttl = 64;
    rip->proto = IPPROTO_ICMP;
    rip->checksum = 0;
    for (int i = 0; i < 4; i++) { rip->src[i] = my_ip[i]; rip->dst[i] = ip->src[i]; }
    rip->checksum = ip_checksum(rip, 20);

    /* ICMP */
    ricmp->type = ICMP_ECHO_REP; ricmp->code = 0;
    ricmp->checksum = 0; ricmp->id = icmp->id; ricmp->seq = icmp->seq;
    for (int i = 0; i < icmp_len; i++) payload[i] = ((char *)(icmp + 1))[i];
    ricmp->checksum = ip_checksum(ricmp, 8 + icmp_len);

    e1000_send_packet(reply, 14 + 20 + 8 + icmp_len);
    serial_puts("ICMP: echo reply sent\r\n");
}

/* ================================================================
 * net_send_ip — 构建并发送 IP 包 (供 TCP 调用)
 * proto: IPPROTO_TCP, data: 传输层数据, dlen: 数据长度
 * ================================================================ */
void net_send_ip(unsigned char proto, unsigned char *dst_ip, void *data, int dlen)
{
    char buf[2048];
    struct eth_hdr *eth = (struct eth_hdr *)buf;
    struct ip_hdr *ip = (struct ip_hdr *)(eth + 1);
    char *payload = (char *)(ip + 1);

    /* Ethernet */
    for (int i = 0; i < 6; i++) { eth->dst[i] = gw_mac[i]; eth->src[i] = my_mac[i]; }
    eth->ethertype = swap16(ETHERTYPE_IP);

    /* IP */
    ip->ver_ihl = 0x45; ip->dscp = 0;
    ip->total_len = swap16(20 + dlen);
    ip->id = swap16(0); ip->flags_frag = 0; ip->ttl = 64;
    ip->proto = proto;
    ip->checksum = 0;
    for (int i = 0; i < 4; i++) { ip->src[i] = my_ip[i]; ip->dst[i] = dst_ip[i]; }
    ip->checksum = ip_checksum(ip, 20);

    /* Payload */
    for (int i = 0; i < dlen; i++) payload[i] = ((char *)data)[i];

    e1000_send_packet(buf, 14 + 20 + dlen);
}

/* ================================================================
 * net_poll — 处理收到的包 (由 e1000 poll 或 timer 调用)
 * ================================================================ */
void net_poll(void *packet, int len)
{
    if (len < 14) return;
    struct eth_hdr *eth = (struct eth_hdr *)packet;
    unsigned short etype = swap16(eth->ethertype);

    if (etype == ETHERTYPE_ARP) {
        arp_reply(packet);
    } else if (etype == ETHERTYPE_IP) {
        struct ip_hdr *ip = (struct ip_hdr *)(eth + 1);
        if (ip->proto == IPPROTO_ICMP) {
            icmp_reply(packet);
        } else if (ip->proto == IPPROTO_TCP) {
            /* 路由到 TCP 层 */
            extern void tcp_input(void *, int);
            tcp_input(packet, len);
        }
    }
}
