# Day 52: /proc 伪文件系统

## 学习目标
- 实现 ext2 一级间接块 (i_block[12] → 256 个数据块)
- 实现快速符号链接 (≤60B 存储在 inode 中)
- 实现文件删除 (释放 blocks + inode + 目录项)

## 基础知识

### 间接块寻址
```
直接块: i_block[0..11]  → 12KB
间接块: i_block[12]     → 指向 256-条目块 → 256KB
最大文件: 268KB
```

## 代码文件
- `fs/ext2.c` — ext2 完整实现

## QEMU 验证
```bash
cd course/day50
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
