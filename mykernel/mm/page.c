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

    /* 第三步: PD[0] 从 2MB 大页切换为 PT 指针 */
    pd[0] = (unsigned long)pt | 0x07;   /* PS=0: 指向 PT, U/S=1 */
    pd[1] = 0x200087;                    /* 2MB huge page @ 2MB, U/S=1 */

    /* 第四步: CR3 刷新 */
    write_cr3((unsigned long)pml4);
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
 * 简化实现: 对用户空间第一个 4KB 页(0x300000)做 COW
 * cow_page_ref: 物理页引用计数 (1=独占, >1=共享只读)
 * ================================================================ */

static int cow_page_ref = 0;       /* 引用计数 */
static unsigned long cow_phys = 0;  /* COW 物理页地址 */
static unsigned long cow_pte_idx = 0; /* COW 页在 new_pt 中的索引 */

/* cow_setup — 将用户区从 2MB 大页切换为 4KB 页, 设置 COW */
void cow_setup(void)
{
    extern void *alloc_page(void);

    /* 分配一个新的 PT 页给 2MB-4MB 区域 */
    unsigned long *new_pt = (unsigned long *)alloc_page();
    if (!new_pt) return;

    /* 填充 4KB 身份映射 (512 个条目, 覆盖 2MB-4MB) */
    for (int i = 0; i < 512; i++) {
        new_pt[i] = (0x200000 + i * 0x1000) | 0x07;  /* P=1 W=1 U/S=1 */
    }

    /* 切换 pd[1] 从 2MB 大页 → PT 指针 */
    pd[1] = (unsigned long)new_pt | 0x07;  /* PS=0, P=1 W=1 U/S=1 */

    /* 用户空间 0x300000 = new_pt 中的索引 256 */
    cow_pte_idx = (0x300000 - 0x200000) / 0x1000;  /* = 256 */
    cow_phys = new_pt[cow_pte_idx] & ~0xFFFUL;     /* = 0x300000 */
    cow_page_ref = 1;

    /* 刷新 TLB */
    write_cr3((unsigned long)pml4);

    serial_puts("cow: setup va=0x300000 phys=");
    print_hex64(cow_phys);
    serial_puts(" pte_idx=");
    print_hex64(cow_pte_idx);
    serial_puts(" ref=1\r\n");
}

/* cow_fork_handler — fork 时标记用户页为只读, 增加引用计数 */
void cow_fork_handler(void)
{
    if (cow_page_ref == 0) return;

    /* 获取用户空间 PT */
    unsigned long *user_pt = (unsigned long *)(pd[1] & ~0xFFFUL);

    /* 父进程 PTE: 清除 Writable 位 (bit 1) */
    user_pt[cow_pte_idx] &= ~2UL;   /* 变为只读 */
    cow_page_ref = 2;               /* 父子共享 */

    /* 刷新 TLB */
    write_cr3((unsigned long)pml4);

    serial_puts("cow: fork done, ref=2, PTE read-only\r\n");
}

/* handle_cow_fault — #PF 时处理 COW (由 entry.S pf_handler 调用)
 * cr2: 缺页地址
 * 返回: 0=已处理(可重试), -1=无法处理
 */
int handle_cow_fault(unsigned long cr2)
{
    /* 只处理用户空间 COW 区域 (0x300000 - 0x301000) */
    if (cr2 < 0x300000 || cr2 >= 0x301000 || cow_page_ref <= 1)
        return -1;  /* 不是 COW 页或不需要复制 */

    /* 获取用户空间 PT */
    unsigned long *user_pt = (unsigned long *)(pd[1] & ~0xFFFUL);
    unsigned long old_pte = user_pt[cow_pte_idx];
    unsigned long old_phys = old_pte & ~0xFFFUL;

    /* 分配新物理页 */
    extern void *alloc_page(void);
    unsigned long *new_page = (unsigned long *)alloc_page();
    if (!new_page) return -1;

    /* 拷贝旧页内容到新页 */
    unsigned long *old_page = (unsigned long *)old_phys;
    for (int i = 0; i < 512; i++)
        new_page[i] = old_page[i];

    /* 更新 PTE: 指向新页, Writable=1 */
    user_pt[cow_pte_idx] = (unsigned long)new_page | 0x07;

    /* 减少旧页引用计数 */
    cow_page_ref--;

    /* 刷新 TLB */
    write_cr3((unsigned long)pml4);

    serial_puts("cow: fault handled, old_phys=");
    print_hex64(old_phys);
    serial_puts(" new_phys=");
    print_hex64((unsigned long)new_page);
    serial_puts(" ref=");
    print_hex64(cow_page_ref);
    serial_puts("\r\n");

    return 0;  /* 成功 */
}
