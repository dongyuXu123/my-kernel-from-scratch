.section .rodata
.globl busybox_elf_start, busybox_elf_end
busybox_elf_start:
.incbin "../initfs/busybox"
busybox_elf_end:
