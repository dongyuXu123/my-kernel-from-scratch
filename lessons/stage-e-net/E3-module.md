# Day 23: 内核模块加载器

> 对照: reference/linux-7.1/kernel/module/main.c

## 1. 目标
- 解析 ET_REL ELF, 分配 SHF_ALLOC 段, 符号解析, R_X86_64_64 重定位
- 调用 init_module 入口
- **验收**: `insmod hello_mod.o` 打印 "Hello from asm module!"

## 2. 关键实现
- `kernel/module.c` — sys_insmod(): 读模块文件→解析section headers→分配内存→复制段/BSS→符号解析(查ksymtab)→重定位→调init
- `include/module.h` — register_kernel_symbol/lookup_symbol
- 重定位: R_X86_64_64(写8B绝对地址), R_X86_64_PC32(写4B相对偏移)

## 3. 内核符号表
```c
register_kernel_symbol("serial_puts", serial_puts);
register_kernel_symbol("print_hex64", print_hex64);
```
