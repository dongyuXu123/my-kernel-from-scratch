/*
 * head.S — Day 2: 64 位入口 + GDT + 页表数据
 *
 * 职责(仅 Day 2 需要):
 *   1. start64: 64 位入口, 死循环(验证长模式切换)
 *   2. 数据段: GDT, 页表(供 boot.S 使用)
 */

/* ============================
 * 1. 64 位代码段 — 死循环
 * ============================ */
.code64
.section .text

.globl start64
start64:
    /*
     * 此时已进入 64 位长模式:
     * - CS = 0x08 (64-bit code segment)
     * - 身份映射前 4MB
     *
     * Day 2 仅验证长模式切换成功。
     * gdb 在此断点可确认 RIP 在 64 位代码段。
     */
    hlt
    jmp     start64

/* ============================
 * 2. 数据段 — GDT, 页表
 * ============================ */
.section .data

    /* ---- 2.1 GDT(64 位模式) ---- */
    .align 16
    .globl gdt
gdt:
        .quad 0x0000000000000000        /* null 描述符 */
        .quad 0x0020980000000000        /* 1: 64-bit CS, DPL=0 (0x08) */
        .quad 0x0000920000000000        /* 2: 数据段, DPL=0 (0x10) */
        .quad 0x0020F80000000000        /* 3: 64-bit CS, DPL=3 (0x1B) */
        .quad 0x0000F20000000000        /* 4: 数据段, DPL=3 (0x23) */
        .quad 0, 0                      /* 5: TSS 占位 */
    .globl gdt_end
gdt_end:

    .align 4
    .globl gdt_ptr
gdt_ptr:
        .word gdt_end - gdt - 1         /* GDT limit */
        .quad gdt                        /* GDT base(64bit linear) */

    /* ---- 2.2 页表(4 级, 各 4KB) ---- */
    .align 4096
    .globl pml4
pml4:
        .skip 4096
    .globl pdp
pdp:
        .skip 4096
    .globl pd
pd:
        .skip 4096
