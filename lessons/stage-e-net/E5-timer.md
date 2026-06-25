# Day 25: 定时器中断 (PIT + PIC)

> 对照: reference/linux-7.1/drivers/clocksource/i8253.c

## 1. 目标
- PIC 重映射(IRQ0→32), PIT 初始化(100Hz)
- 定时器中断处理(EOI + ticks)
- **验收**: 定时器触发打印 ticks

## 2. 关键实现
- `arch/x86/pic.c` — ICW1-4 初始化, pic_enable_irq 取消屏蔽
- `arch/x86/pit.c` — pit_init: 通道0, mode3, divisor=1193182/hz
- `arch/x86/entry.S` — timer_handler: incq ticks, EOI, iretq
- `arch/x86/idt.c` — fill_idt_entry(32, timer_handler, 0)

## 3. 中断门 vs 陷阱门
- 64位中断门: gate[5]=0x8E (P=1,DPL=0,Type=E)
- EOI: 写0x20到PIC主片(端口0x20)告知中断处理完毕
