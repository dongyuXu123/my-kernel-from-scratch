.section .rodata
.align 8
.globl mod_elf_start
mod_elf_start:
.incbin "initfs/hello_mod.o"
.globl mod_elf_end
mod_elf_end:
