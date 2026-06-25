/*
 * arch/x86/switch.S — 上下文切换(汇编)
 *
 * 对照: reference/linux-7.1/arch/x86/entry/entry_64.S (__switch_to_asm)
 *
 * switch_to(prev, next):
 *   prev in %rdi, next in %rsi
 *   保存 callee-saved 寄存器到 prev->rsp,
 *   从 next->rsp 恢复, 切换栈。
 */

.code64
.section .text

/*
 * switch_to(struct task_struct *prev, struct task_struct *next)
 * 简单版: 只保存/恢复 RSP(调度点已保存其他寄存器)
 */
.globl switch_to
switch_to:
    /* 保存 prev RSP */
    movq    %rsp, (%rdi)       /* prev->rsp = rsp */
    /* 恢复 next RSP */
    movq    (%rsi), %rsp       /* rsp = next->rsp */
    ret
