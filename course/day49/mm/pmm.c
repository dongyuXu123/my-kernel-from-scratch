/*
 * mm/pmm.c — 物理内存管理器(bitmap 页分配器)
 *
 * 对照: reference/linux-7.1/mm/memblock.c (early alloc)
 *
 * 内存布局(QEMU 默认 128MB):
 *   heap_start → 0x10D000+
 *   位图在 BSS 中(默认全零=全部空闲),只需标记保留页。
 *
 * alloc_page: O(n)线性扫描, B3 Buddy System 替代
 */

#include "kernel.h"
#include "mm.h"

/* -------- head.S 中的全局符号 -------- */
extern unsigned long heap_addr;
extern unsigned long heap_end_val;
extern unsigned int  total_pages;
extern unsigned int  free_page_cnt;
extern unsigned char  bitmap[];       /* 位图基址(在 BSS) */

/* 链接器定义的 kernel BSS 结束标记 */
extern unsigned char _kernel_end[];

#define PAGE_SHIFT 12
#define PAGE_SIZE  4096

/* ================================================================
 * pmm_init — 初始化物理内存管理器
 * ================================================================ */
void pmm_init(void)
{
    unsigned long heap = (unsigned long)_kernel_end;
    unsigned long heap_end = 0x8000000;  /* 128MB QEMU 默认 */

    heap_addr = heap;
    heap_end_val = heap_end;

    /* 总页数 */
    total_pages = (unsigned int)(heap_end >> PAGE_SHIFT);
    free_page_cnt = total_pages;

    /* 标记保留页: 位 0..(heap/PAGE_SIZE-1) = 已用 */
    unsigned int reserved = (unsigned int)(heap >> PAGE_SHIFT);
    free_page_cnt -= reserved;

    /* 逐位置 1: 位图默认全零, 逐位置 1 */
    for (unsigned int i = 0; i < reserved; i++) {
        /* bts 等价: bitmap[i/8] |= (1 << (i%8)) */
        unsigned int byte_idx = i / 8;
        unsigned int bit_idx  = i % 8;
        bitmap[byte_idx] |= (1 << bit_idx);
    }
}

/* ================================================================
 * alloc_page — 扫描位图找第一个空闲页
 * 返回物理地址, 失败返回 0
 * ================================================================ */
void *alloc_page(void)
{
    unsigned int total = total_pages;

    for (unsigned int i = 0; i < total; i++) {
        unsigned int byte_idx = i / 8;
        unsigned int bit_idx  = i % 8;

        if (!(bitmap[byte_idx] & (1 << bit_idx))) {
            /* 空闲! 标记已用 */
            bitmap[byte_idx] |= (1 << bit_idx);
            free_page_cnt--;
            return (void *)((unsigned long)i << PAGE_SHIFT);
        }
    }
    return 0;  /* 内存耗尽 */
}

/* ================================================================
 * free_page — 释放 4K 物理页
 * addr: 物理地址
 * ================================================================ */
void free_page(void *addr)
{
    unsigned long bit = (unsigned long)addr >> PAGE_SHIFT;
    unsigned int byte_idx = (unsigned int)bit / 8;
    unsigned int bit_idx  = (unsigned int)bit % 8;

    bitmap[byte_idx] &= ~(1 << bit_idx);
    free_page_cnt++;
}
