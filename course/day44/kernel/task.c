/*
 * kernel/task.c — 任务初始化
 *
 * 对照: reference/linux-7.1/kernel/fork.c
 *
 * fork/yield/exit 的汇编逻辑在 arch/x86/entry.S 中,
 * 此文件只提供 C 级别的任务初始化。
 */

#include "kernel.h"
#include "sched.h"

/* head.S 全局符号 */
extern unsigned long pml4[], stack_top;

/* ================================================================
 * init_tasks — 初始化任务结构 (含 cr3 + kstack)
 * ================================================================ */
void init_tasks(void)
{
    unsigned long pml4_pa = (unsigned long)pml4;
    unsigned long kstack = (unsigned long)&stack_top;

    current_task = &tasks[0];
    tasks[0].next = &tasks[1];
    tasks[0].cr3  = pml4_pa;
    tasks[0].kstack_top = kstack;

    tasks[1].next = &tasks[0];
    tasks[1].cr3  = pml4_pa;
    tasks[1].kstack_top = kstack;
}

/* ================================================================
 * prepare_child_task — fork 子进程准备 (分配 PML4 + 内核栈)
 * 返回: 子进程内核栈顶 (指向 iretq 帧的 RSP)
 *
 * 对照: reference/linux-7.1/kernel/fork.c (copy_process)
 * ================================================================ */
unsigned long prepare_child_task(void)
{
    extern unsigned long clone_kernel_pml4(void);
    extern void *alloc_page(void);

    /* 分配子进程内核栈 (4KB) */
    void *kstack = alloc_page();
    if (!kstack) return 0;
    unsigned long kstack_top = (unsigned long)kstack + 4096;

    /* 克隆页表 */
    unsigned long child_cr3_val = clone_kernel_pml4();
    if (!child_cr3_val) return 0;

    /* 设置 child_cr3/kstack 全局变量(供 entry.S 读取) */
    extern unsigned long child_cr3, child_kstack;
    child_cr3   = child_cr3_val;
    child_kstack = kstack_top;

    /* 初始化子进程 task_struct */
    tasks[1].cr3        = child_cr3_val;
    tasks[1].kstack_top = kstack_top;

    serial_puts("fork: child cr3=");
    print_hex64(child_cr3_val);
    serial_puts(" kstack=");
    print_hex64(kstack_top);
    serial_puts("\r\n");

    return kstack_top;
}
