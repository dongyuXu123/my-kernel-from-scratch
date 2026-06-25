# Day 46: Copy-on-Write Fork

## 学习目标
- 理解 COW 原理：fork 后父子共享物理页(只读)，写时触发 #PF 复制
- 实现 `cow_setup()` — 将用户区 2MB 大页切换为 4KB 细粒度映射
- 实现 `cow_fork_handler()` — fork 时标记 PTE 只读，ref_count=2
- 实现 `handle_cow_fault()` — #PF 时分配新页、拷贝数据、更新 PTE

## 基础知识

### COW 流程
```
fork():
  1. 父进程用户 PTE → 清除 Writable 位 (只读)
  2. cow_page_ref = 2 (两个进程共享)
  
子进程写用户页:
  3. #PF (尝试写只读页)
  4. handle_cow_fault():
     a. alloc_page() 分配新物理页
     b. 复制旧页数据到新页
     c. 更新 PTE 指向新页 (Writable=1)
     d. cow_page_ref-- → 1 (独占)
  5. iretq 重试写操作 → 成功
```

## 代码文件
- `mm/page.c` — cow_setup(), cow_fork_handler(), handle_cow_fault()
- `arch/x86/entry.S` — #PF handler 调用 handle_cow_fault()
- `kernel/main.c` — start_kernel 调用 cow_setup()

## QEMU 验证
```bash
cd course/day46
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
