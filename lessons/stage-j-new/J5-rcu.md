# J5: RCU 同步机制 (Day 55)

## 对照源码
- `reference/linux-7.1/kernel/rcu/update.c`

## 学习目标
1. RCU 原理: Read-Copy-Update — 读不加锁, 写延迟释放
2. 宽限期: synchronize_rcu 等待所有 CPU 经历一次调度
3. API: rcu_read_lock/unlock (空), rcu_assign_pointer, rcu_dereference

## 代码导读
- `kernel/rcu.c`: rcu_read_lock/unlock, synchronize_rcu, rcu_assign_pointer/dereference

## 验证
```bash
cd course/day55 && make
qemu-system-x86_64 -kernel mykernel.elf -smp 2 -display none -serial stdio -no-reboot -no-shutdown
```
