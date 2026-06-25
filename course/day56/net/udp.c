/*
 * net/udp.c — UDP 协议 + DHCP 客户端 (Day 51)
 *
 * 对照: reference/linux-7.1/net/ipv4/udp.c
 */

#include "kernel.h"

/* UDP 首部 */
struct udp_hdr {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned short length;
    unsigned short checksum;
} __attribute__((packed));

/* DHCP 消息类型 */
#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_ACK      5

static unsigned char dhcp_state = 0;  /* 0=init, 1=discover sent, 2=bound */

/* udp_send — 发送 UDP 包 */
void udp_send(unsigned short src, unsigned short dst, void *data, int len) {
    serial_puts("UDP: send src="); print_hex64(src);
    serial_puts(" dst="); print_hex64(dst);
    serial_puts(" len="); print_hex64(len);
    serial_puts("\r\n");
    /* 简化: 通过 e1000 直接发送 (实际需 IP 封装) */
}

/* dhcp_discover — 发送 DHCP DISCOVER */
void dhcp_discover(void) {
    serial_puts("DHCP: sending DISCOVER\r\n");
    dhcp_state = 1;
    /* 构建 DHCP 包并发送 */
    unsigned char dhcp_pkt[300];
    for (int i = 0; i < 300; i++) dhcp_pkt[i] = 0;
    dhcp_pkt[0] = 1;    /* BOOTREQUEST */
    dhcp_pkt[1] = 1;    /* Ethernet */
    dhcp_pkt[2] = 6;    /* MAC length */
    dhcp_pkt[10] = 0x63; dhcp_pkt[11] = 0x82;  /* Magic cookie */
    dhcp_pkt[12] = 0x53; dhcp_pkt[13] = 0x63;
    /* Option 53: DHCP DISCOVER */
    dhcp_pkt[14] = 53; dhcp_pkt[15] = 1; dhcp_pkt[16] = DHCP_DISCOVER;
    dhcp_pkt[17] = 255;  /* End */

    udp_send(68, 67, dhcp_pkt, 18);
}

/* dhcp_init — 初始化 DHCP 客户端 */
void dhcp_init(void) {
    serial_puts("DHCP: client initialized\r\n");
}
