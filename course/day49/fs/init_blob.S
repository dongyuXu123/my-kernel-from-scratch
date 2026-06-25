/*
 * init_blob.S — 嵌入 /init ELF 二进制
 *
 * 由 initfs/init 编译后 .incbin 嵌入
 */

.section .rodata
.align 8
.globl init_elf_start
init_elf_start:
.incbin "initfs/init"
.globl init_elf_end
init_elf_end:
