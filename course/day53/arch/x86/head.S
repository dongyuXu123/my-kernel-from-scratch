/*
 * head.S - mykernel 64 位入口 + 汇编辅助函数
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/head_64.S (startup_64)
 *
 * 职责:
 *   1. start64: 64位入口, 清零段寄存器, 设内核栈, call start_kernel()
 *   2. CPU 指令包装(c内联不了): inb/outb, lgdt/lidt/ltr, cr3/msr 读写
 *
 * 数据段: GDT, IDT, TSS, 页表(PML4/PDP/PD/PT), 内核栈
 */

/* ============================
 * 1. 64 位代码段
 * ============================ */
.code64
.section .text

/* ---- CPU 指令包装(供 C 调用) ---- */

/*
 * outb(port, value) — 写 1 字节到 I/O 端口
 * port in %edi, value in %esi
 */
.globl outb
outb:
    movl    %edi, %edx
    movl    %esi, %eax
    outb    %al, %dx
    ret

/*
 * inb(port) — 从 I/O 端口读 1 字节
 * port in %edi, 返回 in %al
 */
.globl inb
inb:
    movl    %edi, %edx
    inb     %dx, %al
    ret

/*
 * outl(port, val) — 写 4 字节到 I/O 端口
 */
.globl outl
outl:
    movl    %edi, %edx
    movl    %esi, %eax
    outl    %eax, %dx
    ret

/*
 * inl(port) — 从 I/O 端口读 4 字节
 */
.globl inl
inl:
    movl    %edi, %edx
    inl     %dx, %eax
    ret

/*
 * lgdt_wrapper(gdt_ptr) — 加载 GDT
 * gdt_ptr in %rdi(指向 10 字节: 2B limit + 8B base)
 */
.globl lgdt_wrapper
lgdt_wrapper:
    lgdt    (%rdi)
    ret

/*
 * lidt_wrapper(idt_ptr) — 加载 IDT
 */
.globl lidt_wrapper
lidt_wrapper:
    lidt    (%rdi)
    ret

/*
 * ltr_wrapper(selector) — 加载任务寄存器
 * selector in %di
 */
.globl ltr_wrapper
ltr_wrapper:
    ltr     %di
    ret

/*
 * write_cr3(val) — 写 CR3(刷新 TLB)
 * val in %rdi
 */
.globl write_cr3
write_cr3:
    movq    %rdi, %cr3
    ret

/*
 * read_cr2() — 读 CR2(缺页地址)
 * 返回 in %rax
 */
.globl read_cr2
read_cr2:
    movq    %cr2, %rax
    ret

/*
 * write_msr(msr, val) — 写 MSR
 * msr in %edi, val in %rsi
 */
.globl write_msr
write_msr:
    movl    %edi, %ecx
    movq    %rsi, %rax
    movq    %rsi, %rdx
    shrq    $32, %rdx
    wrmsr
    ret

/*
 * read_msr(msr) — 读 MSR
 * msr in %edi, 返回 in %rax
 */
.globl read_msr
read_msr:
    movl    %edi, %ecx
    rdmsr
    shlq    $32, %rdx
    orq     %rdx, %rax
    ret

/*
 * enter_user_mode_asm(rip) — 从内核切到 ring3
 * rip in %rdi(用户代码入口地址)
 */
.globl enter_user_mode_asm
enter_user_mode_asm:
    pushq   $0x23                      /* SS = 用户数据段 */
    pushq   $0x390000                  /* RSP = 用户栈(3.5MB, 避开 /init) */
    pushq   $0x3202                    /* RFLAGS(IF=1, IOPL=3) */
    pushq   $0x1B                      /* CS = 用户代码段 */
    pushq   %rdi                       /* RIP = 用户入口 */
    iretq

/* ============================
 * 2. start64 — 64 位入口
 * ============================ */
.globl start64
start64:
    /* 清零段寄存器(用 64 位 flat 模式, NULL selector) */
    xorl    %eax, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    %eax, %ss

    /* 设内核栈 */
    lea     stack_top(%rip), %rsp

    /* 调用 C 内核主函数 */
    call    start_kernel

    /* start_kernel() 不应返回; 如果返回则死循环 */
.hang:
    hlt
    jmp     .hang

/* ============================
 * 3. 数据段 — GDT, 页表, 栈
 * ============================ */
.section .data

	/* ---- 3.1 GDT(64 位模式) ---- */
	.align 16
	.globl gdt
gdt:
	    .quad 0x0000000000000000        /* null 描述符 */
	    .quad 0x0020980000000000        /* 1: 64-bit CS, DPL=0 (0x08) */
	    .quad 0x0000920000000000        /* 2: 数据段, DPL=0 (0x10) */
	    .quad 0x0020F80000000000        /* 3: 64-bit CS, DPL=3 (0x1B) */
	    .quad 0x0000F20000000000        /* 4: 数据段, DPL=3 (0x23) */
	    /* 5: TSS 描述符(0x28), 16 字节, 运行时填充 */
	    .quad 0, 0
	.globl gdt_end
gdt_end:

	.align 4
	.globl gdt_ptr
gdt_ptr:
	    .word gdt_end - gdt - 1         /* GDT limit */
	    .quad gdt                        /* GDT base(64bit linear) */

	/* ---- 3.2 TSS(只需 RSP0 字段) ---- */
	.align 16
	.globl tss
tss:
	    .skip 104                       /* x86-64 TSS */

	/* ---- 3.3 IDT 表(256 项 × 16 字节 = 4K) ---- */
	.align 16
	.globl idt
idt:
	    .skip 4096

	.align 8
	.globl idt_ptr
idt_ptr:
	    .word 4095                      /* limit = 256*16 - 1 */
	    .quad idt                        /* base */

	/* ---- 3.4 页表(4 级, 各 4KB) ---- */
	.align 4096
	.globl pml4
pml4:
	    .skip 4096
	.globl pdp
pdp:
	    .skip 4096
	.globl pd
pd:
	    .skip 4096

	.align 4096
	.globl pt
pt:
	    .skip 4096

	/* ---- 3.5 任务/调度器变量 ---- */
	.align 8
	.globl tasks
tasks:          .skip 96            /* 3 tasks × 32 字节 (rsp+cr3+kstack+next) */
	.globl current_task
current_task:   .quad 0
	.globl child_rsp
child_rsp:      .quad 0
	.globl child_cr3
child_cr3:      .quad 0
	.globl child_kstack
child_kstack:   .quad 0

	/* 内存管理器变量 */
	.align 8
	.globl heap_addr
heap_addr:      .quad 0
	.globl heap_end_val
heap_end_val:   .quad 0
	.globl total_pages
total_pages:    .long 0
	.globl free_page_cnt
free_page_cnt:  .long 0
	.globl free_areas
free_areas:     .skip 88            /* 11 × 8 字节链表头 */
	.globl slab_page
slab_page:      .quad 0
	.globl slab_free
slab_free:      .quad 0

	/* ============================
	 * 4. BSS 段 — 栈 + 位图
	 * ============================ */
	.section .bss
	.align 16
	.globl stack_bottom
stack_bottom:
	    .skip 16384                     /* 16KB 内核栈 */
	.globl stack_top
stack_top:

	.align 4096
	.globl bitmap
bitmap:
	/* heap_start 由 _kernel_end(链接器定义)替代 */
heap_start:
