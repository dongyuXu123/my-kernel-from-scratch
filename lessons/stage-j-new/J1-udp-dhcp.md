# J1: UDP + DHCP (Day 51)

## 对照源码
- `reference/linux-7.1/net/ipv4/udp.c`

## 学习目标
1. UDP 首部: src_port, dst_port, length, checksum
2. DHCP 流程: DISCOVER → OFFER → REQUEST → ACK
3. DHCP 包格式: BOOTREQUEST/BOOTREPLY + options (53=DHCP type)

## 代码导读
- `net/udp.c`: udp_send, dhcp_discover, dhcp_init

## 验证
```bash
cd course/day51 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
