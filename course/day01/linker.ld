/*
 * linker.ld - Day 1: 32 位最小 ELF (仅 boot.S 死循环)
 *
 * QEMU -kernel 需要 multiboot + 32 位入口
 */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .note : { *(.note.*) }

    .multiboot : { KEEP(*(.multiboot)) }

    .text : { *(.text) *(.text.*) }

    .rodata : { *(.rodata) *(.rodata.*) }

    . = ALIGN(4096);
    .data : { *(.data) *(.data.*) }

    . = ALIGN(4096);
    .bss : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* BSS 结束标记(heap 从此开始) */
    . = ALIGN(4096);
    _kernel_end = .;
}
