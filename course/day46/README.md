# Day 46: Copy-on-Write Fork

## 学习目标
- COW 多页: 64页 (0x300000-0x400000) 引用计数数组
- cow_setup(): 分配用户空间 PT, 切换 pd[1]→4KB 页
- cow_fork_handler(): fork 时标记全用户区只读
- handle_cow_fault(): #PF 分配新页+拷贝+更新PTE

## 代码文件
- `mm/page.c` — COW 全部逻辑
- `arch/x86/entry.S` — #PF handler 调用 handle_cow_fault()
- `kernel/main.c` — cow_setup() 在 setup_pagetables 后调用

## QEMU 验证
```bash
cd course/day46 && make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
