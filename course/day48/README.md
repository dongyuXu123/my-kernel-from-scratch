# Day 48: TCP 完整三次握手

## 学习目标
- 实现 TCP 状态机 (CLOSED → SYN_SENT → ESTABLISHED)
- SYN 包发送 + SYN-ACK 接收验证 + ACK 发送
- 序列号管理 + 重传超时 (简化)

## 基础知识

### 三次握手
```
Client                   Server
  |─── SYN seq=x ─────→|
  |←─ SYN-ACK seq=y, ack=x+1 ─|
  |─── ACK ack=y+1 ────→|
  |<════ DATA ═══════>|
```

## 代码文件
- `net/tcp.c` — TCP 状态机 + 握手
- `net/net.c` — IP/以太网层

## QEMU 验证
```bash
cd course/day48
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
