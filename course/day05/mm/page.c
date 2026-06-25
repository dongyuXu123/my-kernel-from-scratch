/*
 * mm/page.c — 页表管理
 *
 * 对照: reference/linux-7.1/arch/x86/mm/init.c (init_mem_mapping)
 *
 * setup_pagetables:
 *   1. 填充 PT[0..511] — 前 2MB 用 4KB 页身份映射
 *   2. 设置 PML4/PDP/PD 的 U/S=1 让 ring3 能访问
 *   3. PD[0] 从 2MB 大页切换到 PT 指针
 *   4. CR3 刷新
 */

#include "kernel.h"

/* -------- head.S 中的全局符号 -------- */
extern unsigned long pml4[];
extern unsigned long pdp[];
extern unsigned long pd[];
extern unsigned long pt[];

/* ================================================================
 * setup_pagetables — 一次性页表初始化
 * ================================================================ */
void setup_pagetables(void)
{
    /* 第一步: 填充 PT[0..511] */
    for (int i = 0; i < 512; i++) {
        pt[i] = (i * 0x1000) | 0x07;  /* P=1 W=1 U/S=1 */
    }

    /* 第二步: PML4[0] 和 PDP[0] 设 U/S=1 */
    pml4[0] |= 0x04;
    pdp[0] |= 0x04;

	/* 第三步: PD[0..63] = 2MB huge pages (0-128MB identity) */
	pd[0] = (unsigned long)pt | 0x07;   /* PS=0: 指向 PT, U/S=1 */
	for (int i = 1; i < 64; i++) {
	    pd[i] = (i * 0x200000) | 0x87;  /* 2MB huge page, U/S=1 */
	}

	/* 第四步: CR3 刷新 */
	write_cr3((unsigned long)pml4);
	serial_puts("page: identity mapped 0-128MB\r\n");
}

/* ================================================================
 * map_mmio — 映射物理 MMIO 区域(2MB 大页)
 * phys: 物理地址(将向下对齐到 2MB)
 * ================================================================ */
void map_mmio(unsigned long phys)
{
    unsigned long base = phys & ~0x1FFFFFUL;  /* 2MB 对齐 */

    /* 计算各级索引 */
    unsigned long pml4_idx = (base >> 39) & 0x1FF;
    unsigned long pdp_idx  = (base >> 30) & 0x1FF;
    unsigned long pd_idx   = (base >> 21) & 0x1FF;

    /* 确保 PDP 条目指向 PD(复用同一个 PD) */
    if (pdp_idx > 0 && pdp[pdp_idx] == 0) {
        pdp[pdp_idx] = (unsigned long)pd | 0x03;  /* P=1 W=1 */
    }

    /* 设置 PD 条目为 2MB 大页 */
    pd[pd_idx] = base | 0x83;  /* P=1 W=1 PS=1(2MB) U/S=0 */

    write_cr3((unsigned long)pml4);  /* 刷新 TLB */

    serial_puts("map_mmio: phys=");
    print_hex64(phys);
    serial_puts(" pdp=");
    print_hex64(pdp_idx);
    serial_puts(" pd=");
    print_hex64(pd_idx);
    serial_puts("\r\n");
}

/* ================================================================
 * clone_kernel_pml4 — 为新进程分配 PML4，拷贝内核映射
 * 返回: 新 PML4 的物理地址
 *
 * 对照: reference/linux-7.1/kernel/fork.c (dup_mm → copy_page_range)
 * ================================================================ */
unsigned long clone_kernel_pml4(void)
{
    /* 分配一个物理页作为新 PML4 */
    extern void *alloc_page(void);
    unsigned long *new_pml4 = (unsigned long *)alloc_page();
    if (!new_pml4) return 0;

    unsigned long new_pa = (unsigned long)new_pml4;

    /* 拷贝内核空间的 PML4 条目 (共享 PDP/PD/PT) */
    unsigned long *src = (unsigned long *)pml4;
    for (int i = 0; i < 512; i++) {
        new_pml4[i] = src[i];
    }

    serial_puts("clone_pml4: new cr3=");
    print_hex64(new_pa);
    serial_puts("\r\n");

    return new_pa;
}

/* ================================================================
 * COW (Copy-on-Write) — Day 46
 *
 * 对照: reference/linux-7.1/mm/memory.c (copy_page_range, do_wp_page)
 *
 * 支持多页: 用户空间 0x300000-0x400000 (64 页 × 4KB)
 * cow_refs[i]: 第 i 页的引用计数
 * ================================================================ */

#define COW_START_VA  0x300000
#define COW_PAGES     64              /* 64 pages = 256KB user space */
#define COW_PTE_BASE  ((COW_START_VA - 0x200000) / 0x1000)  /* PTE index in PT */

static int cow_refs[COW_PAGES];       /* 每页引用计数 */
static unsigned long *cow_pt;         /* 用户空间 PT 页指针 */

/* cow_setup — 将用户区从 2MB 大页切换为 4KB 页, 初始化 COW */
void cow_setup(void)
{
    extern void *alloc_page(void);
    cow_pt = (unsigned long *)alloc_page();
    if (!cow_pt) return;

    /* 填充 4KB 身份映射 (512 条目, 覆盖 2MB-4MB) */
    for (int i = 0; i < 512; i++)
        cow_pt[i] = (0x200000 + i * 0x1000) | 0x07;

    /* 切换 pd[1] 从 2MB 大页 → PT 指针 */
    pd[1] = (unsigned long)cow_pt | 0x07;

    /* 初始化引用计数 (用户区 64 页) */
    for (int i = 0; i < COW_PAGES; i++)
        cow_refs[i] = 1;  /* 独占 */

    write_cr3((unsigned long)pml4);
    serial_puts("cow: setup ");
    print_hex64(COW_PAGES);
    serial_puts(" pages at ");
    print_hex64(COW_START_VA);
    serial_puts("\r\n");
}

/* cow_fork_handler — fork 时标记所有用户页为只读 */
void cow_fork_handler(void)
{
    if (!cow_pt) return;

    for (int i = 0; i < COW_PAGES; i++) {
        cow_pt[COW_PTE_BASE + i] &= ~2UL;  /* 清除 Writable */
        cow_refs[i] = 2;                    /* 父子共享 */
    }

    write_cr3((unsigned long)pml4);
    serial_puts("cow: fork done, ");
    print_hex64(COW_PAGES);
    serial_puts(" pages read-only\r\n");
}

/* handle_cow_fault — #PF 时处理 COW + 按需映射 */
int handle_cow_fault(unsigned long cr2)
{
    /* COW 处理 */
    if (cow_pt && cr2 >= COW_START_VA && cr2 < COW_START_VA + COW_PAGES * 0x1000) {
        int page_idx = (cr2 - 0x200000) / 0x1000 - COW_PTE_BASE;
        if (page_idx >= 0 && page_idx < COW_PAGES && cow_refs[page_idx] > 1) {
            int pte_idx = COW_PTE_BASE + page_idx;
            unsigned long old_phys = cow_pt[pte_idx] & ~0xFFFUL;
            extern void *alloc_page(void);
            unsigned long *np = (unsigned long *)alloc_page();
            if (!np) return -1;
            unsigned long *op = (unsigned long *)old_phys;
            for (int i = 0; i < 512; i++) np[i] = op[i];
            cow_pt[pte_idx] = (unsigned long)np | 0x07;
            cow_refs[page_idx]--;
            write_cr3((unsigned long)pml4);
            return 0;
        }
        return -1;
    }

    /* 按需映射: 用户堆/glibc区域 (2MB 大页) */
    if (cr2 >= 0x500000 && cr2 < 0x8000000) {
        unsigned long base = cr2 & ~0x1FFFFFUL;
        unsigned long pd_idx = base >> 21;
        if (pd_idx < 64 && (pd[pd_idx] & 1) == 0) {
            pd[pd_idx] = base | 0x87;
            write_cr3((unsigned long)pml4);
        }
        return 0;
    }

    return -1;
}
