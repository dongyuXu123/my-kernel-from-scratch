# I5: ext2 间接块 + 符号链接 + 删除 (Day 50)

## 对照源码
- `reference/linux-7.1/fs/ext2/inode.c, symlink.c`

## 学习目标
1. 一级间接块: i_block[12] → 256 数据块 → 最大 268KB 文件
2. 快速符号链接: ≤60 字节目标路径存储在 i_block[] 中 (S_IFLNK)
3. 文件删除: 释放 blocks (bitmap) + 释放 inode + 清理目录项

## 代码导读
- `fs/ext2.c`: ext2_get_block (间接块), ext2_symlink, ext2_unlink
- 间接块: 读 256 个 4B 条目 → 物理块号 → 读/写数据

## 验证
```bash
cd course/day50 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
