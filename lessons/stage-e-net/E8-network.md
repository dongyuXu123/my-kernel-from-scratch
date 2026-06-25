# Day 28: e1000 RX + ARP/ICMP

> 对照: reference/linux-7.1/net/ipv4/

## 1. 目标
- 配置 RX 描述符环, 轮询收包
- ARP 请求→应答(Who has IP?→IP is at MAC)
- ICMP Echo Request→Reply(ping)
- **验收**: ARP/ICMP 处理器正确构建响应包

## 2. 关键实现
- `arch/x86/e1000.c` — RX环(RDBAL/RDLEN/RDH/RDT), e1000_poll: while(rdh!=tail)处理包→net_poll
- `net/net.c` — arp_reply: 检查oper=REQUEST→构建ARP REPLY→e1000_send_packet
- `net/net.c` — icmp_echo_reply: 检查type=8(ECHO_REQ)→构建ECHO_REP→计算IP checksum→发送

## 3. 协议结构(简化)
```
Ethernet: dst[6]|src[6]|ethertype[2]
ARP: htype|ptype|hlen|plen|oper|sha[6]|spa[4]|tha[6]|tpa[4]  (28B)
IP: ver_ihl|dscp|total_len|id|flags|ttl|proto|cksum|src[4]|dst[4]  (20B)
ICMP: type|code|cksum|id|seq|payload  (8B+)
```
