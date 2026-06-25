# Day 13: TSS + ring3 用户态

> 对照: reference/linux-7.1/arch/x86/kernel/process_64.c

## 1. 目标
- **C1**: task_struct{rsp,next}, tasks[2]链表, current_task指针
- **C2**: switch_to(prev,next): 保存prev->rsp, 加载next->rsp
- **C3**: TSS.RSP0=stack_top, enter_user_mode_asm: push SS/RSP/RFLAGS/CS/RIP→iretq
- **C4**: IDT[128]=syscall_entry(DPL=3), rax=sysno分派, iretq返回
- **C5**: fork复制内核栈, 子RAX=0/父RAX=2; yield切换current_task

## 2. 关键实现
-  — struct task_struct{unsigned long rsp, *next}
-  — switch_to: movq %rsp,(%rdi); movq (%rsi),%rsp; ret
-  — tss_init: *(u64*)(tss+4)=stack_top; ltr(0x28)
-  — syscall_entry: push %rax→cmpq分派→.do_read/.do_write/.do_fork→iretq
-  — sys_yield_entry: 保存RSP到current_task→current=next→加载next RSP→iretq

## 3. TSS.RSP0 偏移量 Bug
- **现象**: ring3→ring0 过渡失败, syscall 不触发
- **根因**: RSP0 在 TSS offset 4, 代码错写成 tss64[1](offset 8)
- **修复**: 

## 4. 验收
- fork 后 PCPCPC 交替打印
- ring3 syscall write→串口输出
