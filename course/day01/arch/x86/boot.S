/*
 * boot.S — Day 1: 最小内核框架 (QEMU + GDB + Makefile)
 *
 * 目标: 用 QEMU 加载一个最小 ELF，GDB 能断点到 _start
 * 对照: reference/linux-7.1/arch/x86/boot/header.S
 *
 * 职责:
 *   1. multiboot1 头 (QEMU -kernel 需要)
 *   2. PVH ELF note (QEMU 10.2 需要)
 *   3. _start: 死循环 (只需 QEMU 加载不报错)
 */

/* ============================
 * 1. multiboot1 头(32 位入口)
 * ============================ */
.section .multiboot
.align 4
#define MB_MAGIC    0x1BADB002
#define MB_FLAGS    0x00000003   /* bit0=页对齐, bit1=内存信息 */
#define MB_CHECKSUM -(MB_MAGIC + MB_FLAGS)

multiboot_header:
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM
    .long multiboot_header          /* header_addr */
    .long multiboot_header          /* load_addr */
    .long 0                         /* load_end_addr = 整段 */
    .long 0                         /* bss_end_addr = 不清 bss */
    .long _start                    /* entry_addr */

/* ============================
 * 1b. PVH ELF note(QEMU 10.2 需要)
 * ============================ */
.section .note.Xen, "a", @note
.align 4
    .long 2f - 1f          /* namesz = sizeof("Xen") = 4 */
    .long 3f - 2f          /* descsz = 4 (32-bit entry addr) */
    .long 18               /* type = XEN_ELFNOTE_PHYS32_ENTRY */
1:  .asciz "Xen"           /* name */
2:  .align 4
3:  .long _start            /* desc: 32-bit entry 地址 */

/* ============================
 * 2. 32 位入口: 死循环
 * ============================ */
.section .text
.code32
.global _start
.type _start, @function
_start:
    /*
     * 来到此处时:
     *   - CPU 在 32 位保护模式(CR0.PE=1, CR0.PG=0)
     *   - 身份映射(物理地址 = 线性地址)
     *   - multiboot magic 在 eax, info 指针在 ebx
     *
     * Day 1 不做任何初始化，仅验证 QEMU 加载成功。
     * gdb 可以在此设置断点。
     */
    jmp     _start
