# J4: 自旋锁 + 并行调度 (Day 54)

## 对照源码
- `reference/linux-7.1/kernel/locking/spinlock.c`

## 学习目标
1. spin_lock: xchg 原子操作 (交换并测试) + pause 减少总线争用
2. spin_unlock: mov $0 释放
3. 多核调度: per-CPU runqueue + IPI 负载均衡

## 代码导读
- `kernel/spinlock.c`: spin_lock (xchgl), spin_unlock, spin_trylock

## 验证
```bash
cd course/day54 && make
qemu-system-x86_64 -kernel mykernel.elf -smp 2 -display none -serial stdio -no-reboot -no-shutdown
```
