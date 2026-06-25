# I3: TCP 完整三次握手 (Day 48)

## 对照源码
- `reference/linux-7.1/net/ipv4/tcp_input.c, tcp_output.c`

## 学习目标
1. TCP 状态机: CLOSED → SYN_SENT → ESTABLISHED → FIN_WAIT → CLOSED
2. 三次握手: SYN(seq=x) → SYN-ACK(seq=y, ack=x+1) → ACK(ack=y+1)
3. 序列号管理: ISS, IRS, SND.NXT, SND.UNA, RCV.NXT
4. 校验和: 伪首部 + TCP 首部 + 数据

## 代码导读
- `net/tcp.c`: tcp_connect, tcp_input, tcp_send (完整重写)
- TCP 首部构建: src/dst port, seq/ack, flags, window, checksum

## 验证
```bash
cd course/day48 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
