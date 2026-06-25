# Day 1: 沙盘搭建 — QEMU + GDB + Makefile

> 对照: reference/linux-7.1/Makefile, arch/x86/boot/header.S

## 1. 目标与验收
- **目标**: 搭建编译调试环境，写最小内核（32位入口→长模式→64位死循环）
- **验收**: `make` 编译 `mykernel.elf`; QEMU `-kernel` 加载不报错; gdb 命中 `_start`(此时已在64位长模式)

## 2. 真实源码导读
- `arch/x86/boot/header.S` — 真实内核的 16 位实模式 + PE/EFI 头(bzImage 格式)
- `arch/x86/boot/compressed/head_64.S:startup_32` — 32位入口，建临时页表，切长模式
- `arch/x86/kernel/vmlinux.lds` — 链接脚本(多段布局)

**关键认知**: 真实内核走 bzImage(A1)，我们走 PVH note + 32位入口(简化自 head_64.S)

## 3. 我的实现
- `arch/x86/boot.S` — multiboot头 + PVH note + _start(32位)
- `linker.ld` — 加载到 1MiB, OUTPUT_FORMAT(elf64-x86-64)
- `Makefile` — freestanding 编译, `-m64 -mcmodel=large -mno-red-zone`

## 4. 运行验证
```bash
make && timeout 3 qemu-system-x86_64 -kernel mykernel.elf -display none -serial null -no-reboot -no-shutdown
# exit=124 = timeout 杀死死循环 = 内核正确加载运行
```

## 5. 纠错日志
### PVH note 缺失
- **现象**: QEMU 10.2 报 "Error loading uncompressed kernel without PVH ELF Note"
- **根因**: QEMU 10.2 `-kernel` 优先匹配 PVH note(XEN_ELFNOTE_PHYS32_ENTRY)
- **修复**: 添加 `.note.Xen` 段(namesz=4,descsz=4,type=18,name="Xen",desc=_start)
