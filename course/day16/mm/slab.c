/*
 * mm/slab.c — Slab 分配器(kmalloc/kfree)
 *
 * 对照: reference/linux-7.1/mm/slub.c
 *
 * 简化设计:
 *   单页 slab: 1 页 = 64 个 64 字节对象, 空闲链表管理。
 *   所有 kmalloc 返回 64 字节对象(忽略 size 参数)。
 */

#include "kernel.h"
#include "mm.h"

#define SLAB_OBJ_SIZE 64
#define SLAB_OBJS     64           /* 4096 / 64 = 64 */

/* -------- head.S 中的全局符号 -------- */
extern unsigned long slab_page;    /* slab 页物理地址 */
extern unsigned long slab_free;    /* 空闲链表头 */

/* ================================================================
 * slab_init — 初始化 slab 分配器
 * ================================================================ */
void slab_init(void)
{
    /* 从 buddy 分配 1 页(order=0) */
    char *page = (char *)alloc_pages(0);
    if (!page) {
        serial_puts("slab: no memory\r\n");
        return;
    }

    slab_page = (unsigned long)page;
    slab_free = (unsigned long)page;  /* 第一个对象 */

    /* 链接所有 64 个对象 */
    for (int i = 0; i < SLAB_OBJS - 1; i++) {
        unsigned long *obj = (unsigned long *)(page + i * SLAB_OBJ_SIZE);
        *obj = (unsigned long)(page + (i + 1) * SLAB_OBJ_SIZE);
    }
    /* 最后一个: next = NULL */
    unsigned long *last = (unsigned long *)(page + (SLAB_OBJS - 1) * SLAB_OBJ_SIZE);
    *last = 0;
}

/* ================================================================
 * kmalloc(size) — 分配内核对象
 * 简化: 始终返回 64 字节, 忽略 size
 * ================================================================ */
void *kmalloc(unsigned long size)
{
    (void)size;  /* 未使用 */

    unsigned long obj = slab_free;
    if (!obj)
        return 0;  /* slab 耗尽 */

    /* 从空闲链表取头结点 */
    slab_free = *(unsigned long *)obj;
    return (void *)obj;
}

/* ================================================================
 * kfree(ptr) — 释放内核对象
 * ================================================================ */
void kfree(void *ptr)
{
    if (!ptr)
        return;

    unsigned long obj = (unsigned long)ptr;

    /* 插入到空闲链表头 */
    *(unsigned long *)obj = slab_free;
    slab_free = obj;
}
