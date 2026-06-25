# H4: ext2 写入 (Day 44)

## 对照源码
- `reference/linux-7.1/fs/ext2/ialloc.c, balloc.c, inode.c`

## 学习目标
1. ext2_write_block(): blkdev_write 写块到磁盘
2. ext2_ialloc(): inode bitmap 分配 (找空闲位 → 置位 → 初始化 inode)
3. ext2_balloc(): block bitmap 分配 (跳过保留块 → 置位 → 更新计数)
4. ext2_create(): 目录项格式 [inode(4B)][rec_len(2B)][name_len(1B)][name...]

## 代码导读
- `fs/ext2.c`: ext2_write_inode, ext2_balloc, ext2_ialloc, ext2_create, ext2_write_file
- 存储: ext2_total_blocks, ext2_free_blocks, ext2_free_inodes, ext2_gdesc 全局变量

## 验证
```bash
cd course/day44 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
