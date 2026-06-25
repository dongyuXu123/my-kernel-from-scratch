/*
 * arch/x86/gdt.c — GDT 初始化
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/cpu/common.c (cpu_init)
 *
 * GDT 已在 boot.S 中加载(32位 lgdt)。
 * 此处仅填充 TSS 描述符(GDT[5]), 不再重复加载 GDT。
 */

#include "kernel.h"

extern unsigned char gdt[];
extern unsigned char tss[];

void gdt_init(void)
{
    unsigned long tss_base = (unsigned long)tss;
    unsigned long limit = 103;
    unsigned char *desc = gdt + 40;  /* GDT[5] */

    /* Limit[15:0] */
    desc[0] = limit & 0xFF;
    desc[1] = (limit >> 8) & 0xFF;

    /* Base[15:0] */
    desc[2] = tss_base & 0xFF;
    desc[3] = (tss_base >> 8) & 0xFF;

    /* Base[23:16] */
    desc[4] = (tss_base >> 16) & 0xFF;

    /* Access: P=1, DPL=0, Type=1001 (64-bit TSS available) */
    desc[5] = 0x89;

    /* Flags + Limit[19:16] */
    desc[6] = 0x00;

    /* Base[31:24] */
    desc[7] = (tss_base >> 24) & 0xFF;

    /* Base[63:32] */
    desc[8]  = (tss_base >> 32) & 0xFF;
    desc[9]  = (tss_base >> 40) & 0xFF;
    desc[10] = (tss_base >> 48) & 0xFF;
    desc[11] = (tss_base >> 56) & 0xFF;

    /* Reserved */
    desc[12] = desc[13] = desc[14] = desc[15] = 0;

    /* 注意: 不调用 lgdt_wrapper, boot.S 已加载 GDT */
}
