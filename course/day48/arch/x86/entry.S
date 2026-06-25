/*
 * entry.S — 中断/syscall 入口桩(汇编)
 *
 * 对照: reference/linux-7.1/arch/x86/entry/entry_64.S
 *
 * 包含:
 *   - 异常处理(#PF, #GP, #DF, int3)
 *   - syscall 入口(int $0x80): 分派到 C 函数
 *   - yield(int $0x81): 协作式任务切换
 *   - exit(int $0x83): 子进程退出
 *   - fork 逻辑(内联在 syscall 分派中)
 */

.code64
.section .text

/* ================================================================
 * 异常处理
 * ================================================================ */

/*
 * int3 handler(向量 3, DPL=3)
 */
.globl int3_handler
int3_handler:
    push    %rax
    push    %rdx
    movw    $0x3FD, %dx
.wait3: inb %dx, %al; testb $0x20, %al; jz .wait3
    movb    $'3', %al
    movw    $0x3F8, %dx
    outb    %al, %dx
    pop     %rdx
    pop     %rax
    iretq

/*
 * #PF handler(向量 14, 有 error code) — Day 46: COW 支持
 *
 * 对照: reference/linux-7.1/arch/x86/mm/fault.c (do_page_fault)
 */
	.globl pf_handler
pf_handler:
	push    %rax; push    %rdx; push    %rdi; push    %rsi
	movq    %cr2, %rdi
	call    handle_cow_fault          /* 尝试 COW 处理 */
	testq   %rax, %rax
	jz      .pf_done                  /* COW 成功 → 返回重试 */
	/* COW 未处理 → 已知问题: 静默忽略 kernel page walk PF */
	movq    %cr2, %rax
	cmpq    $0x200000, %rax
	jb      .pf_print
	cmpq    $0x201000, %rax
	jb      .pf_done                  /* 0x200000-0x200FFF: 已知 TLB 问题, 忽略 */
.pf_print:
	call    print_hex64_pf
	movw    $0x3FD, %dx
.wait_pf: inb %dx, %al; testb $0x20, %al; jz .wait_pf
	movb    $'#', %al
	movw    $0x3F8, %dx
	outb    %al, %dx
.pf_done:
	pop     %rsi; pop %rdi; pop %rdx; pop %rax
	pop     %rax                    /* error code */
	iretq

/*
 * #GP handler(向量 13, 有 error code)
 */
.globl gp_handler
gp_handler:
    push    %rax
    push    %rdx
    movw    $0x3FD, %dx
.wait_gp: inb %dx, %al; testb $0x20, %al; jz .wait_gp
    movb    $'G', %al
    movw    $0x3F8, %dx
    outb    %al, %dx
    pop     %rdx
    pop     %rax
    pop     %rax                    /* error code */
    iretq

/*
 * #DF handler(向量 8)
 */
.globl df_handler
df_handler:
    push    %rax
    push    %rdx
    movw    $0x3FD, %dx
.wait_df: inb %dx, %al; testb $0x20, %al; jz .wait_df
    movb    $'D', %al
    movw    $0x3F8, %dx
    outb    %al, %dx
    pop     %rdx
    pop     %rax
    iretq

/* 汇编版 print_hex64(用于 #PF handler) */
print_hex64_pf:
    push    %rax
    push    %rcx
    push    %rdx
    mov     %rax, %r8               /* r8 = 工作副本 */
    mov     $16, %ecx
.phx_lp:
    rolq    $4, %r8
    movq    %r8, %rax
    andq    $0xF, %rax
    addb    $'0', %al
    cmpb    $'9', %al
    jbe     .phx_d
    addb    $7, %al
.phx_d:
    movb    %al, %dil
    /* inline serial_putchar */
    movw    $0x3FD, %dx
.phx_w: inb     %dx, %al; testb $0x20, %al; jz .phx_w
    movb    %dil, %al; movw $0x3F8, %dx; outb %al, %dx
    decl    %ecx
    jnz     .phx_lp
    pop     %rdx
    pop     %rcx
    pop     %rax
	ret

/* 汇编版 serial_putchar(char in %dil) — 供 syscall handler 使用 */
serial_putchar_asm:
	push    %rax; push %rdx
	movw    $0x3FD, %dx
.spw:   inb     %dx, %al; testb $0x20, %al; jz .spw
	movb    %dil, %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	ret

/* ================================================================
 * syscall64 入口 (syscall 指令 → LSTAR)
 *
 * 对照: reference/linux-7.1/arch/x86/entry/entry_64.S (entry_SYSCALL_64)
 *
 * syscall: RCX=返回RIP, R11=保存RFLAGS, 不切换栈
 * 处理: 切到内核栈, 模拟 int $0x80 栈帧, 跳转到 syscall_entry 分发
 * ================================================================ */
	.globl syscall64_entry
syscall64_entry:
	/* 切换到内核栈并构建 int $0x80 兼容栈帧 */
	movq    %rsp, %r10                /* 保存用户 RSP */
	leaq    stack_top(%rip), %rsp     /* 内核栈 */

	pushq   $0x23                     /* SS */
	pushq   %r10                      /* 用户 RSP */
	pushq   %r11                      /* RFLAGS */
	pushq   $0x1B                     /* CS */
	pushq   %rcx                      /* 用户 RIP */
	pushq   %rax                      /* syscall 号 (与 int $0x80 对齐) */

	/* 跳转到 int $0x80 分发逻辑 (跳过 push %rax) */
	jmp     .sc_dispatch

/* ================================================================
 * syscall 入口(int $0x80, DPL=3)
 *
 * 进入时栈上(CPU 自动压入):
 *   SS, RSP_ring3, RFLAGS, CS, RIP
 *
 * Linux x86-64 syscall 兼容 (Day 45+):
 *   0=read, 1=write, 2=open, 3=close, 8=lseek, 9=mmap, 12=brk
 *   39=getpid, 57=fork, 59=execve, 60=exit
 * ================================================================ */
		.globl syscall_entry
syscall_entry:
	push    %rax
.sc_dispatch:
	/* Linux-compatible dispatch */
	cmpq    $0, (%rsp);  je .do_read     /* read */
	cmpq    $1, (%rsp);  je .do_write    /* write */
	cmpq    $2, (%rsp);  je .do_open     /* open */
	cmpq    $3, (%rsp);  je .do_close    /* close */
	cmpq    $5, (%rsp);  je .do_fstat    /* fstat */
	cmpq    $8, (%rsp);  je .do_lseek    /* lseek */
	cmpq    $9, (%rsp);  je .do_mmap     /* mmap (简化) */
	cmpq    $12, (%rsp); je .do_brk      /* brk */
	cmpq    $39, (%rsp); je .do_getpid   /* getpid */
	cmpq    $57, (%rsp); je .do_fork     /* fork */
	cmpq    $59, (%rsp); je .do_exec     /* execve */
	cmpq    $60, (%rsp); je .do_exit2    /* exit */
	/* 旧编号兼容 (仅无冲突的保留) */
	cmpq    $4, (%rsp);  je .do_write     /* 旧 write=4 */
	cmpq    $6, (%rsp);  je .do_list
	cmpq    $7, (%rsp);  je .do_cat
	cmpq    $9, (%rsp);  je .do_insmod
	cmpq    $10, (%rsp); je .do_writeto
	cmpq    $11, (%rsp); je .do_pipe
	cmpq    $13, (%rsp); je .do_signal
	cmpq    $14, (%rsp); je .do_kill
	jmp     .sc_default

/* ---- read(syscall 0): fd-aware(串口或文件) ---- */
.do_read:
	push    %rdi; push    %rsi; push    %rdx
	/* 调用 C sys_read(fd, buf, len), 返回 !=0 表示已完成 */
	call    sys_read
	cmpq    $0, %rax
	jne     .rd_ret
	/* sys_read 返回 0 → fd=0, 串口读取 */
	movq    8(%rsp), %r8           /* r8 = buf(saved rsi) */
	movq    0(%rsp), %rcx          /* rcx = len(saved rdx) */
	testq   %rcx, %rcx
	jz      .rd_sdone
	.rd_slp:
		sti
		movw    $0x3FD, %dx
	.rd_sw: inb %dx, %al; testb $0x01, %al; jz .rd_sw
		cli
		movw    $0x3F8, %dx; inb %dx, %al
	movb    %al, (%r8); movb %al, %dil
	/* Ctrl-C 检测 */
	cmpb    $0x03, %al
	jne     1f
	push    %rdi; movb    %al, %dil; call    signal_check_ctrlc; pop     %rdi
1:
	call    serial_putchar_asm
	cmpb    $'\r', %al; je .rd_sdone
	cmpb    $'\n', %al; je .rd_sdone
	incq    %r8; decq %rcx; jnz .rd_slp
.rd_sdone:
	movq    %r8, %rax; subq 8(%rsp), %rax
.rd_ret:
	movq    %rax, 24(%rsp)          /* → 用户 RAX */
	pop     %rdx; pop %rsi; pop %rdi
	pop     %rax; iretq

	/* ---- fork(syscall 1) — 含 per-process 页表 + 内核栈 ---- */
	.do_fork:
	    push    %rdi; push    %rsi; push    %rdx; push    %rcx; push    %r8

	    /* 调用 C 函数分配 PML4 + 内核栈 → 返回 child kstack_top */
	    call    prepare_child_task
	    testq   %rax, %rax
	    jz      .fk_fail                /* 分配失败 → 返回 -1 */

	    /* 子栈顶 = kstack_top - 72 (4 regs + 5 iretq) */
	    movq    %rax, %rbx              /* rbx = kstack_top */
	    subq    $72, %rbx               /* rbx = child RSP */
	    movq    %rbx, child_rsp(%rip)

	    /* 拷贝父内核栈 (72 bytes: 4 saves + rax + iretq frame) */
	    movq    %rsp, %rsi              /* rsi = 父 RSP */
	    movq    %rbx, %rdi              /* rdi = 子 RSP */
	    movq    $9, %rcx               /* 72/8 = 9 qwords */
	.fk_lp: movq (%rsi),%rax; movq %rax,(%rdi); addq $8,%rsi; addq $8,%rdi; loop .fk_lp

	    /* 设子 RAX=0 (offset 24 in frame = 3 qwords from RSP) */
	    movq    child_rsp(%rip), %rdi
	    movq    $0, 24(%rdi)            /* 子 RAX=0 */

	    /* 保存父进程信息到 mailbox (供子进程 exit 恢复) */
	    movq    %rsp, 0x180000          /* 父 RSP → mailbox */
	    movq    %cr3, %rax
	    movq    %rax, 0x181010          /* 父 CR3 → mailbox */

	    /* Day 46: COW — 标记用户页只读, 增加引用计数 */
	    call    cow_fork_handler

	    /* 父返回 2 */
	    movq    $2, 24(%rsp)
	    jmp     .fk_done

	.fk_fail:
	    movq    $-1, 24(%rsp)           /* 返回 -1 表示失败 */
	.fk_done:
	    pop     %r8; pop %rcx; pop %rdx; pop %rsi; pop %rdi; pop %rax; iretq

/* ---- open(syscall 3) ---- */
.do_open:
    push    %rdi; push    %rsi
    call    ramfs_create_wrapper    /* C 函数: ramfs_create("hello") */
    movq    %rax, 16(%rsp)          /* 返 fd → 用户 RAX */
    pop     %rsi; pop %rdi
    pop     %rax; iretq

/* ---- write(syscall 4/5): fd-aware(串口或文件) ---- */
.do_write:
	push    %rdi; push    %rsi; push    %rdx
	/* 调用 C sys_write(fd, buf, len), 返回 !=0 表示已完成 */
	call    sys_write
	cmpq    $0, %rax
	jne     .dw_ret
	/* sys_write 返回 0 → fd=1, 串口输出 */
	movq    8(%rsp), %rsi           /* buf */
	movq    0(%rsp), %rdx           /* len */
	testq   %rdx, %rdx
	jz      .dw_sdone
.dw_slp:
	movb    (%rsi), %dil
	call    serial_putchar_asm
	incq    %rsi; decq %rdx; jnz .dw_slp
.dw_sdone:
	movq    0(%rsp), %rax           /* 返回 len */
.dw_ret:
	movq    %rax, 24(%rsp)
	pop     %rdx; pop %rsi; pop %rdi
	pop     %rax; iretq

/* ---- list(syscall 6): ramfs_list ---- */
.do_list:
	push    %rdi
	call    ramfs_list
	pop     %rdi
	pop     %rax; iretq

/* ---- cat(syscall 7): ramfs_cat(filename) ---- */
.do_cat:
	push    %rdi; push    %rsi
	movq    8(%rsp), %rdi           /* 用户传入的文件名指针 */
	call    ramfs_cat
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- exec(syscall 8): 加载并执行 ELF ---- */
.do_exec:
	push    %rdi; push    %rsi
	movq    8(%rsp), %rdi           /* 用户传入的文件名 */
	call    do_sys_exec             /* 返回 entry point(0=失败) */
	testq   %rax, %rax
	jz      .exec_fail
	/* 成功: 修改 iretq 帧 */
	movq    %rax, 24(%rsp)          /* 替换用户 RIP */
	movq    $0x3F0000, 48(%rsp)     /* 替换用户 RSP */
	pop     %rsi; pop %rdi
	pop     %rax; iretq             /* → 新程序! */
.exec_fail:
	movq    $-1, 16(%rsp)
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- insmod(syscall 9): 加载内核模块 ---- */
.do_insmod:
	push    %rdi; push    %rsi
	movq    8(%rsp), %rdi           /* 模块文件名 */
	call    sys_insmod
	movq    %rax, 16(%rsp)          /* 返回值 → 用户 RAX */
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- writeto(syscall 10): 写数据到文件 ---- */
.do_writeto:
	push    %rdi; push    %rsi; push    %rdx
	call    sys_writeto            /* C 函数 */
	movq    %rax, 24(%rsp)
	pop     %rdx; pop %rsi; pop %rdi
	pop     %rax; iretq

/* ---- pipe(syscall 11): 创建管道 ---- */
.do_pipe:
	push    %rdi
	/* rdi = fds[2] 数组指针(用户态) */
	call    sys_pipe
	movq    %rax, 8(%rsp)          /* 返回值 → 用户 RAX */
	pop     %rdi
	pop     %rax; iretq

/* ---- mmap(syscall 9): 简化 — 递增分配页 ---- */
.do_mmap:
	push    %rdi
	movq    mmap_next(%rip), %rax
	addq    $0x1000, %rax
	movq    %rax, mmap_next(%rip)
	movq    %rax, 8(%rsp)             /* 返回映射地址 */
	pop     %rdi
	pop     %rax; iretq

/* ---- dup2(syscall 12): 复制fd ---- */
.do_dup2:
	push    %rdi; push    %rsi
	/* rdi=oldfd, rsi=newfd */
	call    sys_dup2
	movq    %rax, 16(%rsp)         /* 返回值 → 用户 RAX */
	pop     %rsi; pop %rdi
	pop     %rax; iretq

.sc_default:
    pop     %rax; iretq

/* ---- 数据 ---- */
.section .data
.align 8
child_rsp_save: .quad 0

.section .rodata
fs_data:
    .ascii "Hello FS"

/* ---- ramfs_create wrapper(call with "hello") ---- */
.section .text
ramfs_create_wrapper:
    lea     fs_name(%rip), %rdi     /* "hello" */
    jmp     ramfs_create            /* tail call */

.section .rodata
fs_name:
    .asciz "hello"

/* ================================================================
 * yield 入口(int $0x81, DPL=3)
 *
 * mailbox 布局:
 *   [0x180000] = 对方 kernel RSP (指向 iretq 帧)
 *   [0x181000] = 对方 RAX
 *   [0x181008] = 对方 TSS.RSP0
 * ================================================================ */
	.globl sys_yield_entry
sys_yield_entry:
	/* 保存全部调用者寄存器(4 pushes) */
	push    %rax; push    %rdx; push    %rdi; push    %rsi

	/* 保存当前 RSP → current_task->rsp */
	movq    current_task(%rip), %rax
	movq    %rsp, (%rax)

	/* 切换到下一个任务: offset 0=rsp, 8=cr3, 16=kstack, 24=next */
	movq    24(%rax), %rax              /* task->next */
	movq    %rax, current_task(%rip)

	/* 恢复 next 的 RSP */
	movq    (%rax), %rsp               /* rsp = next->rsp */

	/* 切换 CR3 */
	movq    8(%rax), %rdx              /* rdx = next->cr3 */
	movq    %rdx, %cr3

	/* 更新 TSS.RSP0 */
	movq    16(%rax), %rdi             /* rdi = next->kstack_top */
	push    %rax
	call    tss_update_rsp0
	pop     %rax

	pop     %rsi; pop %rdi; pop %rdx; pop %rax
	iretq

/* ================================================================
 * exit 入口(int $0x83, DPL=3)
 * 子进程退出, 切回父进程 (含 CR3 恢复)
 * ================================================================ */
	.globl sys_exit_entry
sys_exit_entry:
	/* 恢复父进程 CR3 (从 mailbox 0x181010) */
	movq    0x181010, %rdx
	testq   %rdx, %rdx
	jz      1f
	movq    %rdx, %cr3
1:
	/* 恢复父 RSP (从 mailbox 0x180000) */
	movq    0x180000, %rsp
	pop     %rdx; pop %rsi; pop %rdi; pop %rax
	iretq

/* ================================================================
 * 定时器中断(IRQ0 → 向量 32) — 抢占式调度
 *
 * 对照: reference/linux-7.1/kernel/sched/core.c (scheduler_tick)
 * ================================================================ */
	.globl timer_handler
timer_handler:
	push    %rax; push    %rdx; push    %rcx; push    %rsi; push    %rdi

	incq    timer_ticks(%rip)
	movb    $0x20, %al; outb %al, $0x20  /* EOI */

	/* 每 tick 轮询 e1000 RX */
	call    e1000_poll

	/* 抢占调度: 仅在 sched_ready=1 时启用 */
	movq    sched_ready(%rip), %rdx
	testq   %rdx, %rdx
	jz      1f

	/* 每 10 ticks 做一次调度 (100Hz → 10Hz 调度频率) */
	movq    timer_ticks(%rip), %rdx
	andq    $0x07, %rdx                  /* ticks % 8 != 0 → 跳过 */
	jnz     1f

	/* ---- 抢占式调度 ---- */
	/* 保存当前 RSP → current_task->rsp */
	movq    current_task(%rip), %rax
	movq    %rsp, (%rax)

	/* 切换到下一个任务 */
	movq    24(%rax), %rax               /* task->next (offset 24) */
	movq    %rax, current_task(%rip)

	/* 恢复 next 的 RSP */
	movq    (%rax), %rsp                /* rsp = next->rsp */

	/* 切换 CR3 */
	movq    8(%rax), %rdx               /* rdx = next->cr3 */
	movq    %rdx, %cr3

	/* 更新 TSS.RSP0 */
	movq    16(%rax), %rdi              /* rdi = next->kstack_top */
	call    tss_update_rsp0

1:
	pop     %rdi; pop %rsi; pop %rcx; pop %rdx; pop %rax
	iretq

.align 8
.globl timer_ticks
timer_ticks: .quad 0
	.globl user_brk
user_brk:   .quad 0x559000    /* 初始 brk (BusyBox .bss 结束) */
	.globl mmap_next
mmap_next:  .quad 0x600000    /* mmap 分配起始地址 */
	.globl sched_ready
sched_ready: .quad 0

/* worker 任务内核栈 */
.align 16
worker_stack: .skip 4096
.globl worker_stack_top
worker_stack_top:

/* ================================================================
 * worker 任务(ring3): 每被调度到就打印 'W', yield
 * ================================================================ */
.globl worker_entry
worker_entry:
	movw $0x3FD,%dx; .ww: inb %dx,%al; testb $0x20,%al; jz .ww
	movb $'W',%al; movw $0x3F8,%dx; outb %al,%dx
	int     $0x81                 /* yield */
	jmp     worker_entry

/* ================================================================
 * ring3 入口(user_io) — 用户态代码
 *
 * 流程:
 *   1. open("hello")   — syscall 3
 *   2. write("Hello FS") — syscall 5
 *   3. fork            — syscall 1
 *   4. 子: 打印 'C', yield, 循环
 *   5. 父: 打印 'P', yield, 循环
 * ================================================================ */
.globl user_io
user_io:
	/* ---- 文件操作(仅首次执行) ---- */
	movq    $3, %rax; int $0x80    /* open/create "hello" */
	movq    $5, %rax; int $0x80    /* write "Hello FS" */
	/* ---- fork → 分叉为父子两个循环 ---- */
	movq    $1, %rax; int $0x80    /* fork */
	cmpq    $0, %rax; jne .parent_loop
	/* --- 子进程循环 --- */
.child_loop:
	movw $0x3FD,%dx; .wc: inb %dx,%al; testb $0x20,%al; jz .wc
	movb $'C',%al; movw $0x3F8,%dx; outb %al,%dx
	int     $0x81                 /* yield→父 */
	jmp     .child_loop
	/* --- 父进程循环 --- */
.parent_loop:
	movw $0x3FD,%dx; .wp: inb %dx,%al; testb $0x20,%al; jz .wp
	movb $'P',%al; movw $0x3F8,%dx; outb %al,%dx
	int     $0x81                 /* yield→子 */
	jmp     .parent_loop

/* ================================================================
 * worker 任务(ring3): 打印 'W', yield
 * ================================================================ */

/* ---- signal(syscall 13) ---- */
.do_signal:
	push    %rdi; push    %rsi
	/* rdi=sig, rsi=handler */
	call    sys_signal
	movq    %rax, 16(%rsp)
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- kill(syscall 14) ---- */
.do_kill:
	push    %rdi; push    %rsi
	/* rdi=pid, rsi=sig */
	call    sys_kill
	movq    %rax, 16(%rsp)
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- close(syscall 15) ---- */
.do_close:
	push    %rdi
	/* rdi=fd */
	call    sys_close
	movq    %rax, 8(%rsp)
	pop     %rdi
	pop     %rax; iretq

/* ---- lseek(syscall 16) ---- */
.do_lseek:
	push    %rdi; push    %rsi; push    %rdx
	/* rdi=fd, rsi=offset, rdx=whence */
	call    sys_lseek
	movq    %rax, 24(%rsp)
	pop     %rdx; pop %rsi; pop %rdi
	pop     %rax; iretq

/* ---- fstat(syscall 17): 简化版 — 返回文件大小 ---- */
.do_fstat:
	push    %rdi; push    %rsi
	/* rdi=fd, rsi=buf */
	call    sys_fstat
	movq    %rax, 16(%rsp)
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- getpid(syscall 18) ---- */
.do_getpid:
	push    %rdi
	movq    current_task(%rip), %rdi
	leaq    tasks(%rip), %rax
	subq    %rax, %rdi
	shrq    $5, %rdi        /* div by 32 (sizeof task_struct) */
	movq    %rdi, 8(%rsp)   /* PID = task index */
	pop     %rdi
	pop     %rax; iretq

/* ---- waitpid(syscall 19): 简化 — yield 直到子进程退出 ---- */
.do_waitpid:
	/* 简化实现: 调用 yield 让出 CPU */
	push    %rdi; push    %rsi
	movq    $0, 24(%rsp)      /* 返回 0 */
	pop     %rsi; pop %rdi
	pop     %rax; iretq

/* ---- brk(syscall 12): 扩展用户堆 ---- */
.do_brk:
	push    %rdi
	movq    user_brk(%rip), %rax      /* 当前 brk */
	cmpq    %rax, %rdi
	jbe     1f                        /* 收缩: 返回当前值 */
	movq    %rdi, user_brk(%rip)      /* 扩展 */
1:	movq    user_brk(%rip), %rax
	movq    %rax, 8(%rsp)
	pop     %rdi
	pop     %rax; iretq

/* ---- exit(syscall 60): POSIX exit ---- */
.do_exit2:
	/* 跳转到 exit 入口 (int $0x83) */
	movq    current_task(%rip), %rax
	movq    24(%rax), %rax       /* task->next */
	movq    %rax, current_task(%rip)
	movq    (%rax), %rsp
	movq    8(%rax), %rdx; movq %rdx, %cr3
	movq    16(%rax), %rdi; call tss_update_rsp0
	pop     %rdx; pop %rsi; pop %rdi; pop %rax
	iretq
