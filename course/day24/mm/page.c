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
