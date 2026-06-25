/*
 * hello.S — /bin/hello 独立程序
 *
 * 编译: gcc -static -nostdlib -Wl,-Ttext=0x400000 -Wl,--build-id=none -o hello hello.S
 */

.code64
.section .text
.globl _start
_start:
    leaq    msg(%rip), %rsi
    call    puts
    /* exit */
    movq    $9, %rax
    int     $0x80
.hang:  hlt; jmp .hang

puts:
    push %rdi; push %rdx; push %rsi
    movq (%rsp),%rdi; movq %rdi,%rdx
.sl: cmpb $0,(%rdx); je .sd; incq %rdx; jmp .sl
.sd: subq %rdi,%rdx; movq $4,%rax; movq $1,%rdi; int $0x80
    pop %rsi; pop %rdx; pop %rdi; ret

.section .rodata
msg: .ascii "Hello from /bin/hello!\r\n"
