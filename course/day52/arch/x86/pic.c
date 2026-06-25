/*
 * arch/x86/pic.c — 8259A PIC 重映射
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/i8259.c
 *
 * 把 IRQ0-7 映射到向量 32-39, IRQ8-15 映射到 40-47
 * (避开 CPU 异常向量 0-31)
 */

#include "kernel.h"

void pic_remap(void)
{
    /* ICW1: 初始化 */
    outb(0x20, 0x11);  /* 主片 */
    outb(0xA0, 0x11);  /* 从片 */

    /* ICW2: 基址 */
    outb(0x21, 0x20);  /* 主片: IRQ0→32 */
    outb(0xA1, 0x28);  /* 从片: IRQ8→40 */

    /* ICW3: 级联 */
    outb(0x21, 0x04);  /* 主片 IRQ2 接从片 */
    outb(0xA1, 0x02);  /* 从片接主片 IRQ2 */

    /* ICW4: 8086 模式 */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    /* 屏蔽所有中断 */
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

/* 使能 IRQ line */
void pic_enable_irq(int irq)
{
    unsigned short port = (irq < 8) ? 0x21 : 0xA1;
    unsigned char  mask = inb(port);
    mask &= ~(1 << (irq & 7));
    outb(port, mask);
}
