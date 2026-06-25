# I1: Copy-on-Write Fork (Day 46)

## 对照源码
- `reference/linux-7.1/mm/memory.c` (copy_page_range, do_wp_page)

## 学习目标
1. COW 原理: fork 后父子共享只读页 + #PF 触发复制
2. cow_setup(): 将 2MB 大页切换为 4KB 细粒度 PT → pd[1] 指向新 PT
3. cow_fork_handler(): fork 时清除 PTE Writable 位, ref_count=2
4. handle_cow_fault(): 分配新物理页 → 拷贝数据 → 更新 PTE → ref_count--

## 代码导读
- `mm/page.c`: cow_setup, cow_fork_handler, handle_cow_fault
- `arch/x86/entry.S`: #PF handler 调用 handle_cow_fault, 成功后 iretq 重试

## 验证
```bash
cd course/day46 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
