/*
 * kernel/rcu.c — RCU 同步机制 (Day 55)
 *
 * 对照: reference/linux-7.1/kernel/rcu/update.c
 *
 * 简化 RCU: 单核无抢占环境下的 Read-Copy-Update
 * rcu_read_lock/unlock 是空操作, synchronize_rcu 等待所有 CPU 调度
 */

#include "kernel.h"

static int rcu_gp_count = 0;  /* 宽限期计数 */

/* rcu_read_lock — 进入 RCU 读临界区 (单核下为空) */
void rcu_read_lock(void) { }

/* rcu_read_unlock — 退出 RCU 读临界区 */
void rcu_read_unlock(void) { }

/* synchronize_rcu — 等待所有 CPU 经历一次调度 (宽限期) */
void synchronize_rcu(void)
{
    rcu_gp_count++;
    /* 简化: 等待 ~1ms (100 次 PIT tick) */
    extern unsigned long timer_ticks;
    unsigned long start = timer_ticks;
    while (timer_ticks - start < 10) {
        __asm__ volatile ("pause");
    }
    serial_puts("RCU: grace period ");
    print_hex64(rcu_gp_count);
    serial_puts(" done\r\n");
}

/* rcu_assign_pointer — 赋值 RCU 保护的指针 */
void rcu_assign_pointer(void **ptr, void *new_val)
{
    *ptr = new_val;
}

/* rcu_dereference — 读取 RCU 保护的指针 */
void *rcu_dereference(void **ptr)
{
    return *ptr;
}
