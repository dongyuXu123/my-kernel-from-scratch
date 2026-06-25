# Day 51: UDP 协议 + DHCP 客户端

## 学习目标
- 实现 UDP 首部构建和校验和
- 实现 DHCP DISCOVER → OFFER → REQUEST → ACK
- 自动获取 IP 地址、网关、DNS

## 代码文件
- `net/udp.c` — UDP 协议 (新)
- `net/dhcp.c` — DHCP 客户端 (新)

## QEMU 验证
```bash
cd course/day51
make
timeout 5 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
