/*
 * mm/buddy.c — Buddy System 伙伴分配器
 *
 * 对照: reference/linux-7.1/mm/page_alloc.c (__alloc_pages, __free_one_page)
 *
 * 设计:
 *   11 个 order(0-10): 4K → 4MB。每个 free_area[i] 是单向链表头。
 *   空闲块首 8 字节存 next 指针, 偏移 8 存 order。
 */

#include "kernel.h"
#include "mm.h"

#define MAX_ORDER 10
#define PAGE_SIZE 4096

/* -------- head.S 中的全局符号 -------- */
extern unsigned long heap_addr;
extern unsigned long free_areas[];  /* 11 × 8 字节链表头 */

/* 块头部结构(位于空闲块起始处) */
struct buddy_block {
    struct buddy_block *next;   /* offset 0: 链表 next 指针 */
    int order;                  /* offset 8: 块的 order */
};

/* ================================================================
 * buddy_init — 初始化 Buddy System
 * ================================================================ */
void buddy_init(void)
{
    /* 清零 free_areas */
    for (int i = 0; i <= MAX_ORDER; i++) {
        *(unsigned long *)(free_areas + i) = 0;
    }

    /* 把整个 heap 作为一个大块(order=MAX_ORDER)挂到 free_areas[MAX_ORDER] */
    struct buddy_block *block = (struct buddy_block *)heap_addr;
    block->order = MAX_ORDER;
    block->next  = 0;

    *(struct buddy_block **)(free_areas + MAX_ORDER) = block;
}

/* ================================================================
 * alloc_pages(order) — 分配 2^order 页
 * 返回块物理地址, 失败返回 0
 * ================================================================ */
void *alloc_pages(int order)
{
    if (order < 0 || order > MAX_ORDER)
        return 0;

    /* 从 order 开始向上找第一个非空 free list */
    int cur = order;
    while (cur <= MAX_ORDER && *(struct buddy_block **)(free_areas + cur) == 0)
        cur++;

    if (cur > MAX_ORDER)
        return 0;  /* 内存耗尽 */

    /* 取出链表首块 */
    struct buddy_block *block = *(struct buddy_block **)(free_areas + cur);
    *(struct buddy_block **)(free_areas + cur) = block->next;

    /* 如果当前 order > 目标,递归分裂 */
    while (cur > order) {
        cur--;

        /* 下半部 = 当前块(继续分裂或返回) */
        block->order = cur;
        block->next  = 0;

        /* 上半部 = block_addr + (PAGE_SIZE << cur) */
        unsigned long buddy_addr = (unsigned long)block + (PAGE_SIZE << cur);
        struct buddy_block *buddy = (struct buddy_block *)buddy_addr;
        buddy->order = cur;
        buddy->next  = *(struct buddy_block **)(free_areas + cur);
        *(struct buddy_block **)(free_areas + cur) = buddy;

        /* block 继续指向下半部 */
    }

    return (void *)block;
}

/* ================================================================
 * free_pages(addr, order) — 释放 2^order 页
 * 尝试与 buddy 合并
 * ================================================================ */
void free_pages(void *addr, int order)
{
    if (order < 0 || order > MAX_ORDER)
        return;

    struct buddy_block *block = (struct buddy_block *)addr;
    int cur = order;

    /* 持续尝试向上合并 */
    while (cur < MAX_ORDER) {
        /* 计算 buddy 地址: buddy = addr ^ (PAGE_SIZE << cur) */
        unsigned long buddy_addr = (unsigned long)addr ^ (PAGE_SIZE << cur);
        struct buddy_block *buddy = (struct buddy_block *)buddy_addr;

        /* 在 free_areas[cur] 中查找 buddy */
        struct buddy_block **head = (struct buddy_block **)(free_areas + cur);
        struct buddy_block *prev = 0;
        struct buddy_block *curr = *head;
        int found = 0;

        while (curr) {
            if (curr == buddy) {
                found = 1;
                break;
            }
            prev = curr;
            curr = curr->next;
        }

        if (!found)
            break;  /* buddy 不在空闲链表中, 停止合并 */

        /* 摘除 buddy */
        if (prev)
            prev->next = buddy->next;
        else
            *head = buddy->next;

        /* 合并: addr = min(addr, buddy_addr) */
        if (buddy_addr < (unsigned long)addr)
            addr = (void *)buddy_addr;
        cur++;
    }

    /* 插入合并后的块到 free_areas[cur] */
    block = (struct buddy_block *)addr;
    block->order = cur;
    block->next  = *(struct buddy_block **)(free_areas + cur);
    *(struct buddy_block **)(free_areas + cur) = block;
}
