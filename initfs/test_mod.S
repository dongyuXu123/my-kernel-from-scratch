/*
 * test_mod.S — 最简模块(字符串在 .text 中, 避免 .rodata 重定位)
 */

.code64
.section .text
.globl init_module
init_module:
    movw    $0x3FD, %dx
.w1:    inb     %dx, %al; testb $0x20, %al; jz .w1
    movb    $'M', %al; movw $0x3F8, %dx; outb %al, %dx

    /* 用 call/pop 获取当前地址, 然后加偏移找到字符串 */
    call    .getpc
.getpc:
    popq    %rdi                    /* rdi = .getpc 地址 */
    addq    $msg - .getpc, %rdi     /* rdi = msg 地址 */
    call    serial_puts

    xorl    %eax, %eax
    ret

msg:
    .asciz "Hello from asm module!\r\n"
