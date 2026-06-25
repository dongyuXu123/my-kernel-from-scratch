# J2: /proc 伪文件系统 (Day 52)

## 对照源码
- `reference/linux-7.1/fs/proc/`

## 学习目标
1. /proc 架构: proc_entry 注册 → procfs_read 查找 → read_func 生成内容
2. /proc/meminfo: total_pages, free_page_cnt
3. /proc/cpuinfo: 处理器信息 (固定字符串)
4. /proc/self: 当前进程 PID + 状态

## 代码导读
- `fs/procfs.c`: procfs_register, procfs_read, proc_meminfo/cpuinfo/self_read

## 验证
```bash
cd course/day52 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
