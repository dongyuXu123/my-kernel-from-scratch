# Day 48: TCP 完整三次握手

## 学习目标
- TCP 状态机: CLOSED→SYN_SENT→ESTABLISHED
- net_send_ip(): 统一 Ethernet+IP+TCP 帧构建
- e1000_poll: timer_handler 中轮询 RX
- TCP 校验和 (伪首部+首部+数据)

## 代码文件
- `net/net.c` — net_send_ip + net_poll (ICMP/ARP/TCP 路由)
- `net/tcp.c` — tcp_connect + tcp_input + tcp_send_data
- `arch/x86/entry.S` — timer_handler 调用 e1000_poll

## QEMU 验证
```bash
cd course/day48 && make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
