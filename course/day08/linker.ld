/*
 * linker.ld - mykernel 链接脚本(64 位 ELF)
 *
 * 加载地址 1 MiB (0x100000), PVH 引导。
 * 对照: reference/linux-7.1/arch/x86/kernel/vmlinux.lds
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
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
