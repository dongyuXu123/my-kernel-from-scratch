.section .rodata
.align 8
.globl hello_elf_start
hello_elf_start:
.incbin "initfs/hello"
.globl hello_elf_end
hello_elf_end:
