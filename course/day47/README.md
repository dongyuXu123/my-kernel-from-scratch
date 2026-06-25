# Day 47: ELF PIE 支持

## 学习目标
- 添加 ET_DYN(PIE) ELF 类型支持
- EXEC_BUF_SIZE 从 64KB → 256KB
- 为 BusyBox 等大型静态二进制做准备

## 基础知识

### ET_EXEC vs ET_DYN
```
ET_EXEC (2): 固定加载地址 (绝对地址)
ET_DYN  (3): 位置无关 (PIE), 现代编译器默认 -fPIE
```

### ELF 加载器变更
```c
// 之前: 只接受 ET_EXEC
if (ehdr->e_type != ET_EXEC) return 0;

// Day 47: 同时接受 ET_EXEC 和 ET_DYN
if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) return 0;
```

## 代码文件
- `fs/elf.c` — ET_DYN 类型 + PIE 接受
- `fs/exec.c` — EXEC_BUF_SIZE 256KB

## QEMU 验证
```bash
cd course/day47
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
