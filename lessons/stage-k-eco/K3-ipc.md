# K3: 微内核 IPC (Day 58)

## 对照源码
- `reference/linux-7.1/kernel/ipc/msg.c` (System V 消息队列)

## 学习目标
1. 消息传递: send(pid, msg, len) + recv(pid, buf, len)
2. 消息队列: 固定大小环形缓冲区, FIFO 顺序
3. 宏内核 vs 微内核: IPC 性能对比

## 代码导读
- `kernel/ipc.c`: ipc_send, ipc_recv, ipc_init

## 验证
```bash
cd course/day58 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
