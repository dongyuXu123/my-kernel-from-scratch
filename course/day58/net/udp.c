/*
 * net/udp.c — UDP + DHCP 客户端 (Day 51)
 *
 * 对照: reference/linux-7.1/net/ipv4/udp.c
 */

#include "kernel.h"

/* DHCP 状态: 0=init, 1=discover, 2=request, 3=bound */
static int dhcp_state = 0;

void udp_send(unsigned short src, unsigned short dst, void *data, int len)
{
    /* 构建 UDP 包并通过 IP 层发送 */
    unsigned char buf[1500];
    /* UDP 首部: src_port(2) + dst_port(2) + length(2) + checksum(2) */
    buf[0] = (src >> 8) & 0xFF; buf[1] = src & 0xFF;
    buf[2] = (dst >> 8) & 0xFF; buf[3] = dst & 0xFF;
    buf[4] = ((8 + len) >> 8) & 0xFF; buf[5] = (8 + len) & 0xFF;
    buf[6] = 0; buf[7] = 0;  /* checksum = 0 (简化) */
    for (int i = 0; i < len; i++) buf[8 + i] = ((char*)data)[i];

    extern void net_send_ip(unsigned char, unsigned char*, void*, int);
    unsigned char dst_ip[4] = {255, 255, 255, 255};  /* 广播 */
    net_send_ip(17, dst_ip, buf, 8 + len);
}

void dhcp_discover(void)
{
    if (dhcp_state != 0) return;
    dhcp_state = 1;

    /* 构建 DHCP DISCOVER 包 */
    unsigned char pkt[300];
    for (int i = 0; i < 300; i++) pkt[i] = 0;
    pkt[0] = 1;  /* BOOTREQUEST */
    pkt[1] = 1;  /* Ethernet */
    pkt[2] = 6;  /* HW addr len */
    /* DHCP magic cookie */
    pkt[278] = 0x63; pkt[279] = 0x82; pkt[280] = 0x53; pkt[281] = 0x63;
    /* Option 53: DHCPDISCOVER */
    pkt[282] = 53; pkt[283] = 1; pkt[284] = 1;
    /* Option 55: Parameter Request List */
    pkt[285] = 55; pkt[286] = 4; pkt[287] = 1; pkt[288] = 3; pkt[289] = 6; pkt[290] = 15;
    /* End */
    pkt[291] = 255;

    udp_send(68, 67, pkt, 292);
    serial_puts("DHCP: DISCOVER sent\r\n");
}

void dhcp_init(void)
{
    dhcp_state = 0;
    serial_puts("DHCP: ready\r\n");
}
