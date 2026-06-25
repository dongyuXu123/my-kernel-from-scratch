# Day 41: 多地址空间 (Per-Process Page Tables)

## 学习目标
- 理解进程地址空间隔离的必要性
- 实现 `clone_kernel_pml4()` — 为子进程分配独立 PML4
- 扩展 `task_struct` 添加 `cr3` 和 `kstack_top` 字段
- 修改 `fork()` 分配独立内核栈 + 克隆页表
- 上下文切换时保存/恢复 CR3 + 更新 TSS.RSP0

## 基础知识

### 为什么需要独立地址空间？
当前所有进程共享一套页表(同一个 CR3)。fork 出的父子进程在相同物理内存上运行，互相可读写对方数据——真正的操作系统必须隔离。

### PML4 克隆原理
```
clone_kernel_pml4():
  1. alloc_page() 分配一个物理页
  2. 拷贝内核空间 PML4 条目 (共享原有 PDP/PD/PT)
  3. 返回新 PML4 物理地址作为子进程 cr3
```

### task_struct 扩展
```c
struct task_struct {
    unsigned long rsp;         // 内核栈指针
    unsigned long cr3;         // 页表根 (新)
    unsigned long kstack_top;  // 内核栈顶 (新, 用于 TSS.RSP0)
    struct task_struct *next;
};
```

### 上下文切换新增步骤
- `sys_yield_entry` / `timer_handler`：`movq next->cr3 → %cr3`
- `tss_update_rsp0(next->kstack_top)`：更新 TSS.RSP0

## 代码文件
- `include/sched.h` — task_struct 扩展 (cr3 + kstack_top)
- `mm/page.c` — `clone_kernel_pml4()`
- `kernel/task.c` — `prepare_child_task()` (分配 PML4 + 内核栈)
- `arch/x86/entry.S` — fork/yield/exit 改造 (mailbox 存父 CR3)
- `arch/x86/tss.c` — `tss_update_rsp0()`
- `arch/x86/head.S` — tasks 扩容 (.skip 96), child_cr3/child_kstack 全局变量

## QEMU 验证
```bash
cd course/day41
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: 内核启动，fork 后子进程有独立 cr3 和内核栈
```
