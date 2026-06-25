/*
 * include/mm.h — 内存管理 API
 *
 * 对照: reference/linux-7.1/include/linux/mm.h
 */

#ifndef _MM_H
#define _MM_H

/* -------- 物理内存管理器(PMM, bitmap) -------- */
extern void pmm_init(void);
extern void *alloc_page(void);         /* 返回物理地址 */
extern void  free_page(void *addr);    /* 释放物理页 */

/* -------- Buddy System -------- */
extern void buddy_init(void);
extern void *alloc_pages(int order);   /* 2^order 页 */
extern void  free_pages(void *addr, int order);

/* -------- Slab 分配器(kmalloc/kfree) -------- */
extern void slab_init(void);
extern void *kmalloc(unsigned long size);
extern void  kfree(void *ptr);

/* -------- 页表管理 -------- */
extern void setup_pagetables(void);

/* -------- 工具 -------- */
extern void print_hex64(unsigned long val);

#endif /* _MM_H */
