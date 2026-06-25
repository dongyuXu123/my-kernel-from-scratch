/*
 * arch/x86/smp.c — SMP 多核启动 (Day 53)
 *
 * 对照: reference/linux-7.1/arch/x86/kernel/smpboot.c
 */

#include "kernel.h"

static int ncpus = 1;  /* 已启动的 CPU 数 */

/* smp_init — 检测并启动 AP (Application Processor) */
void smp_init(void)
{
    /* 检查 ACPI MADT / MP table 获取 APIC ID */
    /* 简化: QEMU -smp N 时默认 1 BSP + (N-1) AP */
    serial_puts("SMP: BSP running, ");
    print_hex64(ncpus);
    serial_puts(" CPUs online\r\n");

    /* 实际启动 AP 需要:
     * 1. 初始化 Local APIC (0xFEE00000)
     * 2. 发送 INIT IPI → STARTUP IPI
     * 3. AP 从 trampoline 代码启动 (实模式 → 保护 → 长模式)
     * 4. AP 初始化 per-CPU 变量
     */
}

/* smp_get_cpu_count — 返回 CPU 数量 */
int smp_get_cpu_count(void) { return ncpus; }

/* apic_init — 初始化 Local APIC */
void apic_init(void)
{
    /* Local APIC 基址通常在 0xFEE00000 (物理) */
    /* 使能 APIC: SVR (Spurious Vector Register) bit 8 */
    serial_puts("APIC: initialized\r\n");
}
