# H5: Syscall 扩展 (Day 45)

## 对照源码
- `reference/linux-7.1/fs/file.c (close)`, `fs/read_write.c (lseek)`, `fs/stat.c (fstat)`

## 学习目标
1. 新增 7 个 syscall: close(15), lseek(16), fstat(17), getpid(18), waitpid(19), brk(20), exit(60)
2. lseek whence: SEEK_SET=0, SEEK_CUR=1, SEEK_END=2
3. fstat 简化: buf[0]=文件大小, buf[1]=类型(0=字符设备,1=普通文件)
4. ramfs_get_size(): 供 lseek/fstat 查询文件大小

## 代码导读
- `arch/x86/entry.S`: 7 个新 syscall 分派 + 处理
- `kernel/fd.c`: sys_close, sys_lseek, sys_fstat
- `fs/ramfs.c`: ramfs_get_size

## 验证
```bash
cd course/day45 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
