# Day 26: 设备驱动模型

## 学习目标
1. 理解 x86 CPU 的四种运行模式（实模式、保护模式、兼容模式、长模式）
2. 掌握 4 级页表的建立（PML4→PDP→PD→PT）
3. 理解分页原理：虚拟地址→物理地址的转换过程
4. 实现从 32 位保护模式到 64 位长模式的完整切换

## 基础知识

### x86 CPU 运行模式
```
实模式(16位) → 保护模式(32位) → 兼容模式(过渡态) → 长模式(64位)
```
- **实模式**: 8086 兼容，16 位，1MB 寻址
- **保护模式**: 32 位，4GB 寻址，支持分页
- **长模式**: 64 位，48 位虚拟地址(256TB)，必须开启分页
- **兼容模式**: 32→64 位的过渡态，CS.L=0 但 EFER.LMA=1

### 进入长模式的步骤（不可乱序）
```
1. 建页表 (PML4→PDP→PD)
2. CR3 = PML4 地址 (加载页表基址)
3. EFER.LME = 1 (开启长模式使能)
4. CR4.PAE = 1 (开启物理地址扩展)
5. LGDT (加载 64 位 GDT)
6. CR0.PG = 1 (开启分页 → 同时进入长模式)
7. LJMP 到 64 位代码段
```

### 分页原理：2MB 大页
```
虚拟地址 (48位)
  ├─ PML4 索引 (bit 47:39) → PML4[0] → PDP 物理地址
  ├─ PDP 索引  (bit 38:30) → PDP[0]  → PD 物理地址
  ├─ PD 索引   (bit 29:21) → PD[0]   → 2MB 物理页基址(PS=1)
  └─ 页内偏移  (bit 20:0)  → 2MB 内的偏移
```
使用 2MB 大页(PS=1, bit 7 of PDE)可以一次映射 2MB，不需要 4KB 页表(PT)。

### 为什么需要分页才能进入长模式？
长模式要求分页必须开启(CR0.PG=1)。这是架构设计决定——64 位模式下分页是必须的。

## 代码文件详解

### arch/x86/boot.S — 完整的长模式切换

**页表建立**（身份映射前 4MB）:
```asm
/* PML4 是页表的根，物理地址放在 CR3 中 */
/* PML4[0] = 指向 PDP */  
movl $pdp, %eax       # eax = PDP 的物理地址
orl  $0x03, %eax      # flag: bit0=Present(在内存), bit1=Writable
movl %eax, pml4       # 写入 PML4 第一个条目

/* PDP[0] = 指向 PD */
movl $pd, %eax
orl  $0x03, %eax
movl %eax, pdp

/* PD[0] = 2MB 大页映射 0-2MB */
movl $0x00000083, pd   # 0x83 = 2MB页 + Present + Writable
/*      0x83 = 1000 0011
         bit 7 (PS)=1: 这是 2MB 大页，不是 4KB 页表指针
         bit 1 (RW)=1: 可写
         bit 0 (P)=1:  页存在 */

/* PD[1] = 2MB 大页映射 2-4MB */
movl $0x00200083, pd+8 # 物理基址 = 0x200000 (2MB)
```

**开启长模式**:
```asm
/* CR3 指向 PML4 */
movl $pml4, %eax; movl %eax, %cr3

/* EFER (Extended Feature Enable Register) */
movl $0xC0000080, %ecx  # EFER MSR 地址
rdmsr                    # 读 EFER
orl  $(1<<8), %eax       # bit8 = LME (Long Mode Enable)
wrmsr                    # 写回

/* CR4.PAE (Physical Address Extension) */
movl %cr4, %eax
orl  $(1<<5), %eax       # PAE=1 (长模式前提)
movl %eax, %cr4

/* 加载 GDT (必须包含 64 位代码段描述符) */
lgdt gdt_ptr

/* CR0.PG (Paging) — 最后一击！ */
movl %cr0, %eax
orl  $(1<<31), %eax      # PG=1 → 同时进入长模式！
movl %eax, %cr0

/* ljmp 加载 64 位 CS，正式进入 64 位模式 */
ljmp $0x08, $start64     # 0x08 = GDT 第 1 个条目(内核代码段)
```

**GDT (Global Descriptor Table)**:
```asm
gdt:
    .quad 0                     # 空描述符(必须)
    .quad 0x0020980000000000    # 64位内核代码段(0x08)
    .quad 0x0000920000000000    # 数据段(0x10)
    .quad 0x0020F80000000000    # 64位用户代码段(0x1B) DPL=3
    .quad 0x0000F20000000000    # 用户数据段(0x23) DPL=3
```
- DPL (Descriptor Privilege Level): 0=内核, 3=用户
- 代码段需设置 L=1 (64位模式) 和 D=0
- 数据段设置 S=1 (代码/数据段)

## 验证
```bash
make && make run
# 用 gdb 验证:
# (gdb) break start64
# (gdb) info reg cs rip
# cs = 0x08 (64位代码段), rip 在 64 位范围 (>0x100000)
```

## 对照源码
`reference/linux-7.1/arch/x86/boot/compressed/head_64.S:startup_32`
真实内核的启动流程与本节完全一致。

## QEMU 验证
```bash
cd course/day02
make
timeout 3 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: timeout 退出 (成功进入 64 位长模式并 hlt)
# 用 gdb 确认: break start64, info reg cs (应为 0x08)
```
