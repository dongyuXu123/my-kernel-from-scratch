/*
 * kernel/spinlock.c — 自旋锁 (Day 54)
 *
 * 对照: reference/linux-7.1/kernel/locking/spinlock.c
 */

#include "kernel.h"

typedef volatile unsigned int spinlock_t;

/* spin_lock — 获取自旋锁 (xchg 原子操作) */
void spin_lock(spinlock_t *lock)
{
    while (1) {
        /* xchg %1, %0  — 原子交换 */
        unsigned int val = 1;
        __asm__ volatile (
            "xchgl %0, %1"
            : "+r"(val), "+m"(*lock)
            :
            : "memory"
        );
        if (val == 0) break;  /* 成功获取 */
        __asm__ volatile ("pause");  /* 减少总线争用 */
    }
}

/* spin_unlock — 释放自旋锁 */
void spin_unlock(spinlock_t *lock)
{
    __asm__ volatile (
        "movl $0, %0"
        : "=m"(*lock)
        :
        : "memory"
    );
}

/* spin_trylock — 尝试获取 (不等待) */
int spin_trylock(spinlock_t *lock)
{
    unsigned int val = 1;
    __asm__ volatile (
        "xchgl %0, %1"
        : "+r"(val), "+m"(*lock)
        :
        : "memory"
    );
    return val == 0;
}
