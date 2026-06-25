/*
 * initfs/crt0.S — C 运行时入口 (Day 57)
 *
 * newlib 交叉编译的 C 程序入口点
 * 提供 _start, 调用 main(), 退出时调用 exit syscall
 */

.section .text
.globl _start
_start:
    /* 初始化栈指针 (用户态, 内核已设置) */
    /* 调用 main() */
    call    main

    /* 退出: syscall 60 (exit) */
    movq    $60, %rax
    int     $0x80

    /* 永远不会到这儿 */
    hlt
