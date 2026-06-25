/*
 * net/net.c — 简化网络协议栈(ARP + ICMP)
 *
 * 对照: reference/linux-7.1/net/ipv4/arp.c, net/ipv4/icmp.c
 */

#include "kernel.h"

/* 我们的地址 */
static unsigned char my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static unsigned char my_ip[4]  = {10, 0, 2, 15};

/* e1000 接口 */
extern int e1000_send_packet(void *data, int len);
extern void *e1000_rx_buf;  /* 当前收到包的缓冲区 */
extern int e1000_rx_len;    /* 当前收到包的长度 */

/* ================================================================
 * 工具函数
 * ================================================================ */
static unsigned short swap16(unsigned short x)
    { return (x >> 8) | (x << 8); }
static unsigned short cksum(void *data, int len)
{
    unsigned int sum = 0;
    unsigned short *p = (unsigned short *)data;
    for (int i = 0; i < len / 2; i++) sum += p[i];
    if (len & 1) sum += ((unsigned char *)data)[len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (unsigned short)~sum;
}

/* ================================================================
 * 以太网帧结构
 * ================================================================ */
struct eth_hdr {
    unsigned char  dst[6], src[6];
    unsigned short ethertype;
} __attribute__((packed));

#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IP  0x0800

/* ================================================================
 * ARP(仅支持 IPv4 over Ethernet)
 * ================================================================ */
struct arp_hdr {
    unsigned short htype, ptype;
    unsigned char  hlen, plen;
    unsigned short oper;
    unsigned char  sha[6], spa[4];
    unsigned char  tha[6], tpa[4];
} __attribute__((packed));

#define ARP_REQUEST 1
#define ARP_REPLY   2

static void arp_reply(void *packet)
{
    struct eth_hdr *eth = (struct eth_hdr *)packet;
    struct arp_hdr *arp = (struct arp_hdr *)(eth + 1);

    /* 检查是否是发给我们的 ARP 请求 */
    if (arp->oper != swap16(ARP_REQUEST)) return;
    if (arp->tpa[0] != my_ip[0] || arp->tpa[1] != my_ip[1] ||
        arp->tpa[2] != my_ip[2] || arp->tpa[3] != my_ip[3]) return;

    /* 构建 ARP 应答 */
    char reply[64];
    struct eth_hdr *reth = (struct eth_hdr *)reply;
    struct arp_hdr *rarp = (struct arp_hdr *)(reth + 1);

    /* 以太网头 */
    for (int i = 0; i < 6; i++) {
        reth->dst[i] = eth->src[i];
        reth->src[i] = my_mac[i];
    }
    reth->ethertype = swap16(ETHERTYPE_ARP);

    /* ARP 应答 */
    rarp->htype = swap16(1);
    rarp->ptype = swap16(ETHERTYPE_IP);
    rarp->hlen  = 6;
    rarp->plen  = 4;
    rarp->oper  = swap16(ARP_REPLY);
    for (int i = 0; i < 6; i++) rarp->sha[i] = my_mac[i];
    for (int i = 0; i < 4; i++) rarp->spa[i] = my_ip[i];
    for (int i = 0; i < 6; i++) rarp->tha[i] = arp->sha[i];
    for (int i = 0; i < 4; i++) rarp->tpa[i] = arp->spa[i];

    extern int e1000_send_packet(void *data, int len);
    e1000_send_packet(reply, sizeof(struct eth_hdr) + sizeof(struct arp_hdr));
    serial_puts("ARP: reply sent\r\n");
}

/* ================================================================
 * IPv4 + ICMP
 * ================================================================ */
struct ip_hdr {
    unsigned char  ver_ihl, dscp;
    unsigned short total_len, id, flags_frag, ttl;
    unsigned char  proto;
    unsigned short checksum;
    unsigned char  src[4], dst[4];
} __attribute__((packed));

struct icmp_hdr {
    unsigned char  type, code;
    unsigned short checksum;
    unsigned short id, seq;
} __attribute__((packed));

#define IPPROTO_ICMP 1
#define ICMP_ECHO_REQ 8
#define ICMP_ECHO_REP 0

static void icmp_echo_reply(void *packet)
{
    struct eth_hdr *eth = (struct eth_hdr *)packet;
    struct ip_hdr *ip = (struct ip_hdr *)(eth + 1);
    struct icmp_hdr *icmp = (struct icmp_hdr *)(ip + 1);

    /* 检查是否是发给我们的 ICMP Echo Request */
    if (ip->proto != IPPROTO_ICMP) return;
    if (icmp->type != ICMP_ECHO_REQ) return;
    if (ip->dst[0] != my_ip[0] || ip->dst[1] != my_ip[1] ||
        ip->dst[2] != my_ip[2] || ip->dst[3] != my_ip[3]) return;

    /* payload 长度 */
    int ip_hdr_len = (ip->ver_ihl & 0x0F) * 4;
    int icmp_len = swap16(ip->total_len) - ip_hdr_len;

    /* 构建响应包(以太网 + IP + ICMP + payload) */
    char reply[2048];
    struct eth_hdr *reth = (struct eth_hdr *)reply;
    struct ip_hdr *rip = (struct ip_hdr *)(reth + 1);
    struct icmp_hdr *ricmp = (struct icmp_hdr *)(rip + 1);
    char *payload = (char *)(ricmp + 1);

    /* 以太网 */
    for (int i = 0; i < 6; i++) {
        reth->dst[i] = eth->src[i];
        reth->src[i] = my_mac[i];
    }
    reth->ethertype = swap16(ETHERTYPE_IP);

    /* IP */
    int rip_len = 20 + icmp_len;
    rip->ver_ihl  = 0x45;
    rip->dscp     = 0;
    rip->total_len = swap16(rip_len);
    rip->id       = 0;
    rip->flags_frag = 0;
    rip->ttl      = 64;
    rip->proto    = IPPROTO_ICMP;
    rip->checksum = 0;
    for (int i = 0; i < 4; i++) { rip->src[i] = my_ip[i]; rip->dst[i] = ip->src[i]; }
    rip->checksum = cksum(rip, 20);

    /* ICMP */
    ricmp->type     = ICMP_ECHO_REP;
    ricmp->code     = 0;
    ricmp->checksum = 0;
    ricmp->id       = icmp->id;
    ricmp->seq      = icmp->seq;
    for (int i = 0; i < icmp_len - 8; i++)
        payload[i] = ((char *)(icmp + 1))[i];
    ricmp->checksum = cksum(ricmp, icmp_len);

    extern int e1000_send_packet(void *data, int len);
    e1000_send_packet(reply, 14 + rip_len);
    serial_puts("ICMP: echo reply sent\r\n");
}

/* ================================================================
 * net_poll — 处理收到的包
 * ================================================================ */
void net_poll(void *packet, int len)
{
    if (len < 14) return;
    struct eth_hdr *eth = (struct eth_hdr *)packet;

    switch (swap16(eth->ethertype)) {
    case ETHERTYPE_ARP:
        if (len >= 14 + (int)sizeof(struct arp_hdr))
            arp_reply(packet);
        break;
    case ETHERTYPE_IP:
        if (len >= 14 + (int)sizeof(struct ip_hdr) + (int)sizeof(struct icmp_hdr))
            icmp_echo_reply(packet);
        break;
    }
}
