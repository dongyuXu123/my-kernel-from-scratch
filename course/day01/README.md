# Day 1: 沙盘搭建 — QEMU + GDB + Makefile

## 学习目标
1. 理解 x86 计算机从上电到执行第一条内核指令的完整流程
2. 掌握 ELF 可执行文件格式和链接脚本的编写
3. 搭建 QEMU + GDB 调试环境
4. 写一个能被 QEMU 加载的最小内核（multiboot1 + PVH note + 32位死循环）

## 代码文件
- `arch/x86/boot.S` — 最小 32 位入口：multiboot 头 + PVH note + 死循环
- `linker.ld` — 32 位 ELF 链接脚本
- `Makefile` — 32 位编译 (`-m32`)

## QEMU 验证
```bash
cd course/day01
make
timeout 3 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: 3 秒后 timeout 退出 (exit 124)
# 内核被 QEMU 加载并执行死循环，无串口输出
```
