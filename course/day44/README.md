# Day 44: ext2 写入支持

## 学习目标
- 实现 `ext2_write_block()` — 写块到磁盘
- 实现 `ext2_ialloc()` — 从 inode bitmap 分配 inode
- 实现 `ext2_balloc()` — 从 block bitmap 分配数据块
- 实现 `ext2_create()` — 创建文件 + 目录项
- 实现 `ext2_write_file()` — 按需分配数据块 + 写数据

## 基础知识

### inode 分配
```
ext2_ialloc(mode):
  1. 读 inode bitmap 块
  2. 找第一个空闲位 (bit=0), 置位 (bit=1)
  3. 更新 super.free_inodes_count, group_desc
  4. 初始化 inode (mode, links=1, size=0)
  5. 写回 inode 表
```

### block 分配
```
ext2_balloc():
  1. 读 block bitmap 块
  2. 找第一个空闲位 (跳过 super/desc/inode 保留块)
  3. 置位, 写回, 更新计数
```

### 目录项格式 (ext2_dir_entry)
```
[inode(4B)] [rec_len(2B)] [name_len(1B)] [file_type(1B)] [name...]
```

### ext2_write_file 流程
```
1. 计算 block_idx = offset / 1024
2. 如果 i_block[block_idx]==0 → ext2_balloc() 按需分配
3. 读-改-写: 读 block → 修改 → 写回
4. 更新 inode.i_size
```

## 代码文件
- `fs/ext2.c` — ext2 完整实现 (读 + 写 + inode/block 分配 + 文件创建)

## QEMU 验证
```bash
cd course/day44
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: ext2 挂载后可创建和写入文件
```
