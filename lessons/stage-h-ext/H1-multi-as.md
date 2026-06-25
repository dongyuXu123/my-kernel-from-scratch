# H1: 多地址空间 (Day 41)

## 对照源码
- `reference/linux-7.1/kernel/fork.c` (dup_mm → copy_page_range)
- `reference/linux-7.1/arch/x86/kernel/process_64.c` (__switch_to)

## 学习目标
1. 进程地址空间隔离: per-process PML4 + 独立内核栈
2. task_struct 扩展: cr3 字段 (页表根) + kstack_top (内核栈顶)
3. clone_kernel_pml4(): 分配新 PML4, 拷贝内核映射 (共享 PDP/PD/PT)
4. 上下文切换: 保存/恢复 CR3 + tss_update_rsp0()

## 代码导读
- `mm/page.c`: clone_kernel_pml4() — alloc_page → 拷贝 PML4 条目
- `kernel/task.c`: prepare_child_task() — 分配 PML4 + 内核栈
- `arch/x86/entry.S`: fork/yield/exit 增加 CR3 保存/恢复
- `arch/x86/tss.c`: tss_update_rsp0() — 更新 TSS.RSP0

## 验证
```bash
cd course/day41 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
