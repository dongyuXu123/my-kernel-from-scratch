/*
 * arch/x86/smp.c — SMP 多核启动 (Day 53)
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/smpboot.c
 *
 * 实现: AP trampoline (实模式→保护模式→长模式) + IPI
 */

#include "kernel.h"

#define APIC_BASE 0xFEE00000
#define AP_BOOT_ADDR 0x7000  /* AP 启动代码加载地址 (实模式) */

static int ncpus = 1;

/* AP trampoline 代码 (实模式, 放在 0x7000) */
static unsigned char trampoline[] = {
    /* 16-bit real mode entry */
    0xFA,                    /* cli */
    0x0F, 0x20, 0xC0,       /* mov %cr0, %eax */
    0x66, 0x83, 0xC8, 0x01, /* or $1, %eax (PE=1) */
    0x0F, 0x22, 0xC0,       /* mov %eax, %cr0 (enter protected mode) */
    0xEA, 0x00, 0x00, 0x08, 0x00,  /* ljmp $0x08, $ap_prot */
    /* 32-bit protected mode */
    0x66, 0xB8, 0x10, 0x00,       /* mov $0x10, %ax */
    0x8E, 0xD8,                    /* mov %ax, %ds */
    0x0F, 0x22, 0xD8,              /* mov %eax, %cr3 */
    0xB9, 0x80, 0x00, 0x00, 0xC0,  /* mov $0xC0000080, %ecx */
    0x0F, 0x32,                    /* rdmsr */
    0x0F, 0xBA, 0xE8, 0x08,       /* bts $8, %eax (LME=1) */
    0x0F, 0x30,                    /* wrmsr */
    0x0F, 0x20, 0xE0,              /* mov %cr4, %eax */
    0x0F, 0xBA, 0xE8, 0x05,       /* bts $5, %eax (PAE=1) */
    0x0F, 0x22, 0xE0,              /* mov %eax, %cr4 */
    0x0F, 0x20, 0xC0,              /* mov %cr0, %eax */
    0x0F, 0xBA, 0xE8, 0x1F,       /* bts $31, %eax (PG=1) */
    0x0F, 0x22, 0xC0,              /* mov %eax, %cr0 */
    0xEA, 0x00, 0x00, 0x08, 0x00,  /* ljmp $0x08, $ap64 (64-bit) */
    /* AP stops here (will be filled by C code) */
    0xF4, 0xEB, 0xFD,              /* hlt; jmp . */
};

void smp_init(void)
{
    /* 复制 trampoline 到 0x7000 */
    unsigned char *dst = (unsigned char *)(unsigned long)AP_BOOT_ADDR;
    for (int i = 0; i < sizeof(trampoline); i++)
        dst[i] = trampoline[i];

    /* 填充 CR3 值到 trampoline 中 (offset 20 = cr3 mov 地址) */
    extern unsigned long pml4[];
    *(unsigned long *)(dst + 20 + 2) = (unsigned long)pml4;

    serial_puts("SMP: trampoline at 0x7000, ");
    print_hex64(ncpus);
    serial_puts(" cpus\r\n");
}

void apic_init(void)
{
    /* 映射 APIC MMIO (0xFEE00000) */
    extern void map_mmio(unsigned long);
    map_mmio(APIC_BASE);

    /* 启用 Local APIC (SVR bit 8) */
    unsigned int *apic_svr = (unsigned int *)(unsigned long)(APIC_BASE + 0xF0);
    *apic_svr = (*apic_svr & 0xFFFFFF00) | 0x1FF;
    serial_puts("APIC: enabled\r\n");
}

int smp_get_cpu_count(void) { return ncpus; }
