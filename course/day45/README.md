# Day 45: BusyBox 移植准备 (Syscall 扩展)

## 学习目标
- 新增 7 个系统调用为 BusyBox 做准备
- `close(15)`: 释放文件描述符
- `lseek(16)`: 设置文件读写位置 (SEEK_SET/CUR/END)
- `fstat(17)`: 返回文件大小和类型
- `getpid(18)`: 返回进程 PID
- `waitpid(19)`: 等待子进程 (简化版)
- `brk(20)`: 扩展用户堆 (简化版)
- `exit(60)`: POSIX 标准 exit
- `ramfs_get_size()`: 供 lseek/fstat 查询文件大小

## 基础知识

### 新增 syscall 一览
| 编号 | 名称 | 功能 | Linux 对照 |
|------|------|------|-----------|
| 15 | close | 释放 fd, 标记为未使用 | sys_close (fs/file.c) |
| 16 | lseek | fd 读写位置定位 | sys_lseek (fs/read_write.c) |
| 17 | fstat | 获取文件大小/类型 | sys_newstat (fs/stat.c) |
| 18 | getpid | 返回 task index | sys_getpid (kernel/sys.c) |
| 19 | waitpid | 等待子进程 | sys_wait4 (kernel/exit.c) |
| 20 | brk | 扩展堆 (简化: 始终成功) | sys_brk (mm/mmap.c) |
| 60 | exit | POSIX exit (切回父进程) | sys_exit (kernel/exit.c) |

### lseek whence
```
SEEK_SET = 0: pos = offset
SEEK_CUR = 1: pos += offset
SEEK_END = 2: pos = file_size + offset
```

### BusyBox 还需要什么？
- 完整的 ELF 加载器 (ET_DYN 支持, >64KB)
- 修复 open() 接受文件名参数
- mmap/munmap (malloc 需要)
- 进程等待 (waitpid)

## 代码文件
- `arch/x86/entry.S` — 7 个新 syscall 分派和处理
- `kernel/fd.c` — `sys_close()`, `sys_lseek()`, `sys_fstat()`
- `fs/ramfs.c` — `ramfs_get_size()`

## QEMU 验证
```bash
cd course/day45
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: 7 个新 syscall 可被用户态程序调用
```
