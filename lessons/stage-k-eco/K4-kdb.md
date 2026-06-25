# K4: 内核调试器 KDB (Day 59)

## 对照源码
- `reference/linux-7.1/kernel/debug/kdb/`

## 学习目标
1. 串口命令行: 接收命令 → 解析 → 执行
2. reg: 显示所有寄存器 (rax/rbx/rcx/rdx/rsi/rdi/rsp/rip)
3. x/16x addr: 内存 dump (8 个 qword)
4. bp addr: 断点设置 (简化: 仅记录地址)

## 代码导读
- `kernel/kdb.c`: kdb_show_regs, kdb_mem_dump, kdb_set_bp, kdb_command

## 验证
```bash
cd course/day59 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 串口输入命令: reg, x100000, bp, c
```
