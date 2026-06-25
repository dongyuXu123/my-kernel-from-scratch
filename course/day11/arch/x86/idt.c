/*
 * arch/x86/idt.c — IDT 初始化(C 代码)
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/idt.c (idt_setup_early_traps)
 *
 * 64 位 IDT 门: 每项 16 字节
 *   gate[0-1]:  offset_low
 *   gate[2-3]:  segment (0x08)
 *   gate[4]:    IST (0 for non-exception)
 *   gate[5]:    P(bit7) | DPL(bits6-5) | 0(bit4) | Type(0xE)
 *   gate[6-7]:  offset_mid
 *   gate[8-15]: offset_high + reserved
 */

#include "kernel.h"

extern unsigned char idt[];
extern unsigned char idt_ptr[];

/* -------- entry.S 中的处理函数 -------- */
extern void int3_handler(void);
extern void pf_handler(void);
extern void gp_handler(void);
extern void df_handler(void);
extern void syscall_entry(void);
extern void sys_yield_entry(void);
extern void sys_exit_entry(void);
extern void timer_handler(void);

static void fill_idt_entry(int entry_num, void *handler, int dpl)
{
    unsigned long addr = (unsigned long)handler;
    unsigned char *gate = &idt[entry_num * 16];

    /* offset_low */
    gate[0] = addr & 0xFF;
    gate[1] = (addr >> 8) & 0xFF;

    /* segment = 0x08 */
    gate[2] = 0x08;
    gate[3] = 0x00;

    /* IST=0 */
    gate[4] = 0x00;

    /* P=1, DPL, Type=0xE (64-bit interrupt gate) */
    gate[5] = 0x80 | ((dpl & 3) << 5) | 0x0E;

    /* offset_mid = addr[31:16] */
    gate[6] = (addr >> 16) & 0xFF;
    gate[7] = (addr >> 24) & 0xFF;

    /* offset_high = addr[63:32] */
    gate[8]  = (addr >> 32) & 0xFF;
    gate[9]  = (addr >> 40) & 0xFF;
    gate[10] = (addr >> 48) & 0xFF;
    gate[11] = (addr >> 56) & 0xFF;

    /* reserved */
    gate[12] = 0;
    gate[13] = 0;
    gate[14] = 0;
    gate[15] = 0;
}

void idt_init(void)
{
    /* 异常 */
    fill_idt_entry(3,  int3_handler, 3);   /* int3, DPL=3 */
    fill_idt_entry(8,  df_handler,  0);    /* #DF */
    fill_idt_entry(13, gp_handler,  0);    /* #GP */
    fill_idt_entry(14, pf_handler,  0);    /* #PF */

    /* syscall 向量(DPL=3) */
    fill_idt_entry(128, syscall_entry,   3);  /* int $0x80 */
    fill_idt_entry(129, sys_yield_entry, 3);  /* int $0x81 */
    fill_idt_entry(131, sys_exit_entry,  3);  /* int $0x83 */

    /* 定时器(IRQ0 → 向量 32) */
    fill_idt_entry(32, timer_handler, 0);

    lidt_wrapper(idt_ptr);
}
