# Day 42: 抢占式多任务调度

## 学习目标
- 理解协作式 vs 抢占式调度的区别
- 在定时器中断中实现调度 (每 8 ticks ≈ 80ms 切换一次)
- 配合 Day 41：切换时保存/恢复 CR3 + 更新 TSS.RSP0

## 基础知识

### 抢占式 vs 协作式
```
协作式 (Day 15): 进程主动调用 yield() 让出 CPU
抢占式 (Day 42): 定时器中断强制切换，进程无法垄断 CPU
```

### timer_handler 新增调度逻辑
```
IRQ0 → timer_handler:
  1. push 5 个寄存器 (rax, rdx, rcx, rsi, rdi)
  2. incq timer_ticks; EOI
  3. 每 8 ticks:
     a. current_task->rsp = %rsp          // 保存
     b. current_task = current_task->next // 切换
     c. %rsp = current_task->rsp          // 恢复
     d. %cr3 = current_task->cr3          // 切换页表
     e. tss_update_rsp0(kstack_top)       // 更新 TSS
  4. pop 5 寄存器; iretq
```

### 时间片大小
- PIT = 100Hz → 每 10ms 一次中断
- 每 8 ticks 调度 → 80ms 时间片

## 代码文件
- `arch/x86/entry.S` — `timer_handler` 抢占式调度

## QEMU 验证
```bash
cd course/day42
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: 定时器驱动进程切换，不再需要协作式 yield
```
