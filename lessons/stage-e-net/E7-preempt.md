# Day 27: 抢占式多任务调度

> 对照: reference/linux-7.1/kernel/sched/core.c

## 1. 目标
- 定时器中断中保存/恢复任务 RSP, 切换 current_task
- worker 任务与 /init 交替运行
- **验收**: worker 打印 'W', 定时器自动切换

## 2. 关键实现
- `entry.S` timer_handler: 保存RSP到current_task→current=next→加载next RSP→iretq
- `entry.S` sys_yield_entry: 统一使用 current_task 机制(与timer一致)
- `kernel/main.c`: 在 worker_stack 上预建 iretq 帧(32B padding + 40B frame)

## 3. Stack Frame 布局
```
[RSP+0-31]:  padding (4 个 push 槽)
[RSP+32]:    RIP (worker_entry)
[RSP+40]:    CS (0x1B)
[RSP+48]:    RFLAGS (0x3202)
[RSP+56]:    RSP_ring3 (0x390000)
[RSP+64]:    SS (0x23)
```
