# Day 2: 32→64 位切换 — multiboot + 长模式

> 对照: reference/linux-7.1/arch/x86/boot/compressed/head_64.S

## 1. 目标与验收
- **目标**: 从 32 位保护模式切换到 64 位长模式
- **验收**: gdb 确认 CS=0x08(64位代码段), RIP 在 64 位代码范围

## 2. 真实源码导读
- `head_64.S:startup_32` — 计算加载地址→建4级页表→EFER.LME=1→CR4.PAE=1→CR0.PG=1→ljmp 到 startup_64
- `head_64.S:startup_64` — 清零段寄存器，设置栈，调用 C 函数

## 3. 我的实现
- `arch/x86/boot.S` — _start: 建 2MB 大页身份映射(前4MB)→EFER.LME+SCE→CR4.PAE→lgdt→CR0.PG→ljmp $0x08,$start64
- PML4→PDP→PD(2 entries: 映射 0-2MB, 2-4MB)

## 4. 运行验证
```
# gdb 中:
(gdb) break start64
(gdb) continue
(gdb) info registers cs rip
cs = 0x8, rip = 0x1000c4 (start64 在 64 位代码段)
```

## 5. 关键代码
```asm
movl $0xC0000080, %ecx  # EFER MSR
rdmsr
orl $((1<<8)|1), %eax    # LME=1, SCE=1
wrmsr
movl %cr4, %eax
orl $(1<<5), %eax        # PAE=1
movl %eax, %cr4
movl %cr0, %eax
orl $(1<<31), %eax       # PG=1 (开启分页+长模式)
movl %eax, %cr0
ljmp $0x08, $start64     # 加载64位CS
```
