.code64
.section .text
.globl _start
_start:
    leaq banner(%rip), %rsi; call puts
shell:
    leaq prompt(%rip), %rsi; call puts
    leaq cmd_buf(%rip), %rdi; movq $128, %rsi; call readline
    cmpb $0, cmd_buf(%rip); je shell
    leaq cmd_buf(%rip), %r8; movq %r8, %r9
.f: cmpb $' ',(%r9); je .fe; cmpb $0,(%r9); je .fe; cmpb $'\r',(%r9); je .fe; incq %r9; jmp .f
.fe: movb $0,(%r9); incq %r9; cmpb $' ',(%r9); jne .a; incq %r9
.a: movq %r9, %r11
    leaq c_help(%rip),%rdi; movq %r8,%rsi; call strcmp; testq %rax,%rax; jz .do_help
    leaq c_echo(%rip),%rdi; movq %r8,%rsi; call strcmp; testq %rax,%rax; jz .do_echo
    leaq c_ls(%rip),%rdi; movq %r8,%rsi; call strcmp; testq %rax,%rax; jz .do_ls
    leaq c_cat(%rip),%rdi; movq %r8,%rsi; call strcmp; testq %rax,%rax; jz .do_cat
    leaq c_exec(%rip),%rdi; movq %r8,%rsi; call strcmp; testq %rax,%rax; jz .do_exec
    leaq c_insmod(%rip),%rdi; movq %r8,%rsi; call strcmp; testq %rax,%rax; jz .do_insmod
    jmp .unknown
.do_help: leaq msg_help(%rip),%rsi; call puts; jmp shell
.do_echo: cmpb $0,(%r11); je shell; movq %r11,%rsi; call puts; leaq newline(%rip),%rsi; call puts; jmp shell
.do_ls: movq $6,%rax; int $0x80; jmp shell
.do_cat: cmpb $0,(%r11); je .cu; movq %r11,%rdi; movq $7,%rax; int $0x80; leaq newline(%rip),%rsi; call puts; jmp shell
.cu: leaq msg_cat(%rip),%rsi; call puts; jmp shell
.do_exec: cmpb $0,(%r11); je .eu; movq %r11,%rdi; movq $8,%rax; int $0x80; jmp shell
.eu: leaq msg_exec(%rip),%rsi; call puts; jmp shell
.do_insmod: cmpb $0,(%r11); je .iu; movq %r11,%rdi; movq $9,%rax; int $0x80; jmp shell
.iu: leaq msg_insmod(%rip),%rsi; call puts; jmp shell
.unknown: leaq msg_unknown(%rip),%rsi; call puts; movq %r8,%rsi; call puts; leaq newline(%rip),%rsi; call puts; jmp shell

puts: push %rdi; push %rdx; push %rsi; movq (%rsp),%rdi; movq %rdi,%rdx
.sl: cmpb $0,(%rdx); je .sd; incq %rdx; jmp .sl
.sd: subq %rdi,%rdx; movq $4,%rax; movq $1,%rdi; int $0x80; pop %rsi; pop %rdx; pop %rdi; ret
readline: push %rdi; push %rsi; push %rdx
    movq 16(%rsp),%rsi; movq 8(%rsp),%rdx; movq $0,%rax; movq $0,%rdi; int $0x80
    movq 16(%rsp),%rdi; movb $0,(%rdi,%rax); pop %rdx; pop %rsi; pop %rdi; ret
strcmp: push %rdi; push %rsi
.sc: movb (%rdi),%al; movb (%rsi),%cl; cmpb %cl,%al; jne .sd2
    testb %al,%al; jz .seq; incq %rdi; incq %rsi; jmp .sc
.sd2: movq $1,%rax; jmp .sout
.seq: xorq %rax,%rax
.sout: pop %rsi; pop %rdi; ret

.section .rodata
banner: .ascii "Hello from /init!\r\n"; prompt: .ascii "# "; newline: .ascii "\r\n"
c_help: .asciz "help"; c_echo: .asciz "echo"; c_ls: .asciz "ls"; c_cat: .asciz "cat"
c_exec: .asciz "exec"; c_insmod: .asciz "insmod"
msg_help: .ascii "Commands: help echo ls cat exec insmod\r\n"
msg_unknown: .ascii "?: "; msg_cat: .ascii "Usage: cat <file>\r\n"
msg_exec: .ascii "Usage: exec <prog>\r\n"; msg_insmod: .ascii "Usage: insmod <mod.o>\r\n"
.section .bss
cmd_buf: .skip 128
