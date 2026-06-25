# Day 29: VFS 抽象层 + 文件描述符

> 对照: reference/linux-7.1/include/linux/fs.h

## 1. 目标
- 定义 file_operations/inode/file 抽象
- 统一 fd 表(stdin=串口, stdout=串口, 2+=ramfs)
- ramfs 提供 file_operations 实现
- **验收**: open→read→write 通过 VFS 接口

## 2. 关键实现
- `include/vfs.h` — struct file_operations{read,write}, inode{i_fop,i_private}, file{f_inode,f_pos}
- `fs/vfs.c` — fd_table[16], vfs_open→vfs_read→vfs_write
- `kernel/fd.c` — sys_open/sys_read/sys_write: 检查fd=0→串口, fd≥2→VFS→ramfs
- `fs/ramfs.c` — ramfs_fop_read/ramfs_fop_write 对接 VFS

## 3. fd 表布局
```
fd[0] = (file*)1  → stdin(串口读)
fd[1] = (file*)2  → stdout(串口写)
fd[2..15] = ramfs files
```
