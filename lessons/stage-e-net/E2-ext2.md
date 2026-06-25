# Day 22: ext2 只读文件系统

> 对照: reference/linux-7.1/fs/ext2/

## 1. 目标
- 解析 superblock(magic=0xEF53), group descriptor, inode table
- 目录项遍历查找文件, 直接块指针读取内容
- **验收**: 读取 "Hello ext2!" 内容

## 2. 关键实现
- `fs/ext2.c` — ext2_mount(读superblock验证magic)→ext2_read_inode→ext2_lookup→ext2_read_file
- `fs/ext2_test.c` — ext2_mkfs: 在RAM盘构建完整ext2(超块+组描述符+inode表+目录+文件数据)
- 简化: 仅1024B块, rev0(128B inode), 直接块指针(i_block[0..11])

## 3. 关键结构
```
SuperBlock @ offset 1024: s_magic=0xEF53, s_log_block_size=0(1024B)
GroupDesc(32B): bg_inode_table(bn)
Inode(128B): i_mode, i_size, i_block[15]
DirEntry(变长): inode, rec_len, name_len, name[]
```
