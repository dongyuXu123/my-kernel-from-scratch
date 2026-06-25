# I2: ELF PIE 支持 (Day 47)

## 对照源码
- `reference/linux-7.1/fs/binfmt_elf.c` (load_elf_binary)

## 学习目标
1. ET_EXEC vs ET_DYN: 固定地址 vs 位置无关 (PIE)
2. ELF 加载器: 同时接受 ET_EXEC 和 ET_DYN 类型
3. EXEC_BUF_SIZE: 64KB → 256KB (支持更大二进制)

## 代码导读
- `fs/elf.c`: 添加 ET_DYN 常量, 修改类型检查
- `fs/exec.c`: 增大 EXEC_BUF_SIZE

## 验证
```bash
cd course/day47 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
