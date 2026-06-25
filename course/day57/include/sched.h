/*
 * include/sched.h — 任务/调度 API
 *
 * 对照: reference/linux-7.1/include/linux/sched.h
 *
 * task_struct: rsp(内核栈指针) + cr3(页表根) + kstack_top(内核栈顶) + next(链表)
 */

#ifndef _SCHED_H
#define _SCHED_H

/* -------- 任务结构 -------- */
struct task_struct {
    unsigned long rsp;              /* 保存的内核栈指针 */
    unsigned long cr3;              /* 进程页表根 (PML4 物理地址) */
    unsigned long kstack_top;       /* 内核栈顶 (TSS.RSP0 用) */
    struct task_struct *next;       /* 链表 next 指针 */
};

/* -------- 全局任务变量(head.S 中定义) -------- */
extern struct task_struct tasks[2];
extern struct task_struct *current_task;

/* -------- 调度器 API -------- */
extern void sched_init(void);
extern void schedule(void);
extern void switch_to(struct task_struct *prev, struct task_struct *next);

/* -------- 任务 API -------- */
extern unsigned long sys_fork(void);   /* fork syscall 实现 */
extern void sys_exit(void);         /* exit syscall 实现 */
extern void init_tasks(void);       /* 初始化任务 */

/* -------- 用户态入口(汇编, head.S) -------- */
extern void enter_user_mode_asm(void *user_rip);

#endif /* _SCHED_H */
