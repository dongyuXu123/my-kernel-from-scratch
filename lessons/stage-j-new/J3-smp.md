# J3: SMP 多核启动 (Day 53)

## 对照源码
- `reference/linux-7.1/arch/x86/kernel/smpboot.c`

## 学习目标
1. APIC 架构: Local APIC (0xFEE00000) + I/O APIC
2. SMP 启动: BSP → INIT IPI → STARTUP IPI → AP trampoline
3. AP trampoline: 实模式 → 保护模式 → 长模式 → C 入口

## 代码导读
- `arch/x86/smp.c`: smp_init, apic_init, smp_get_cpu_count

## 验证
```bash
cd course/day53 && make
qemu-system-x86_64 -kernel mykernel.elf -smp 2 -display none -serial stdio -no-reboot -no-shutdown
```
