# H2: 抢占式调度 (Day 42)

## 对照源码
- `reference/linux-7.1/kernel/sched/core.c` (scheduler_tick)

## 学习目标
1. 抢占 vs 协作: 定时器中断强制切换 (每 8 ticks ≈ 80ms)
2. timer_handler 调度: 保存 RSP → 切换 current_task → 恢复 RSP+CR3
3. 配合多地址空间: 切换时同步更新 TSS.RSP0

## 代码导读
- `arch/x86/entry.S`: timer_handler — 5 寄存器保存, EOI, 每 8 ticks 调度
- 调度粒度: PIT 100Hz, 8 ticks = 80ms 时间片

## 验证
```bash
cd course/day42 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
