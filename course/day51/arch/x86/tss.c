/*
 * arch/x86/tss.c — TSS 初始化
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/cpu/common.c (cpu_init)
 *
 * 64位 TSS: 只需 RSP0(offset 4) + I/O位图基址(offset 102)
 */

#include "kernel.h"

extern unsigned char tss[];
extern unsigned char stack_top[];

void tss_init(void)
{
    /* RSP0 在 TSS offset 4(64-bit), 不是 offset 8! */
    *(unsigned long *)(tss + 4) = (unsigned long)stack_top;

    /* I/O 位图基址 >= limit(103) → 允许所有 I/O */
    unsigned short *iomap_base = (unsigned short *)(tss + 102);
    *iomap_base = 103;

    /* 加载 TSS(选择子 0x28) */
    ltr_wrapper(0x28);

    /* 初始化 yield mailbox 父进程 RSP0 */
    unsigned long *mb_rsp0 = (unsigned long *)0x181008;
    *mb_rsp0 = (unsigned long)stack_top;
}

/* ================================================================
 * tss_update_rsp0 — 更新 TSS.RSP0 (上下文切换时调用)
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/process_64.c (__switch_to)
 * ================================================================ */
void tss_update_rsp0(unsigned long kstack_top)
{
    *(unsigned long *)(tss + 4) = kstack_top;
}
