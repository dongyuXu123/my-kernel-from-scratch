/*
 * kernel/sched.c — 调度器
 *
 * 对照: reference/linux-7.1/kernel/sched/core.c (schedule)
 */

#include "kernel.h"
#include "sched.h"

void sched_init(void)
{
    /* 初始化任务链表 */
    current_task = &tasks[0];
    tasks[0].next = &tasks[1];
    tasks[1].next = &tasks[0];
}
