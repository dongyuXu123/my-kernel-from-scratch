/*
 * arch/x86/entry.S — Day 03-14: 最小中断处理 (Stage AB)
 *
 * 对照: reference/linux-7.1/arch/x86/entry/entry_64.S
 *
 * 只包含基础异常/中断 handler。无 syscall 分派 (Day 15+ 加入)。
 */

.code64
.section .text

/* ---- int3 handler ---- */
	.globl int3_handler
int3_handler:
	push    %rax; push    %rdx
	movb    $'3', %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	iretq

/* ---- #PF handler (Day 03-08: 仅打印 CR2, Day 09+ 加入 COW) ---- */
	.globl pf_handler
pf_handler:
	push    %rax; push    %rdx
	movq    %cr2, %rax
	call    print_hex64_pf
	movw    $0x3FD, %dx
.wait_pf: inb %dx, %al; testb $0x20, %al; jz .wait_pf
	movb    $'#', %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	pop     %rax
	iretq

/* ---- #GP handler ---- */
	.globl gp_handler
gp_handler:
	push    %rax; push    %rdx
	movw    $0x3FD, %dx
.wait_gp: inb %dx, %al; testb $0x20, %al; jz .wait_gp
	movb    $'G', %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	pop     %rax
	iretq

/* ---- #DF handler ---- */
	.globl df_handler
df_handler:
	push    %rax; push    %rdx
	movw    $0x3FD, %dx
.wait_df: inb %dx, %al; testb $0x20, %al; jz .wait_df
	movb    $'D', %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	iretq

/* ---- Default handler ---- */
	.globl default_handler
default_handler:
	push    %rax; push    %rdx
	movw    $0x3FD, %dx
.wait_d: inb %dx, %al; testb $0x20, %al; jz .wait_d
	movb    $'?', %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	iretq

/* ---- print_hex64_pf (用于 #PF) ---- */
print_hex64_pf:
	push    %rax; push    %rdx; push    %rcx
	movq    %rax, %rcx; movq    $16, %rdx
.ph_lp:	rolq    $4, %rcx; movb    %cl, %al
	andb    $0x0F, %al; addb    $'0', %al
	cmpb    $'9', %al; jbe     .ph_d
	addb    $7, %al
.ph_d:	push    %rcx
	movw    $0x3FD, %dx
.ph_w:	inb     %dx, %al; testb $0x20, %al; jz .ph_w
	pop     %rcx
	movw    $0x3F8, %dx; outb %al, %dx
	decq    %rdx; jnz .ph_lp
	pop     %rcx; pop %rdx; pop %rax
	ret

/* ---- serial_putchar_asm ---- */
	.globl serial_putchar_asm
serial_putchar_asm:
	push    %rax; push    %rdx
	movw    $0x3FD, %dx
.spw:	inb     %dx, %al; testb $0x20, %al; jz .spw
	movb    %dil, %al; movw $0x3F8, %dx; outb %al, %dx
	pop     %rdx; pop %rax
	ret

/* ---- syscall 入口 (int $0x80) — Day 03-14: 仅返回 -1 ---- */
	.globl syscall_entry
syscall_entry:
	movq    $-1, %rax
	iretq

/* ---- timer_handler (minimal) ---- */
	.globl timer_handler
timer_handler:
	push    %rax; push    %rdx
	incq    timer_ticks(%rip)
	movb    $0x20, %al; outb %al, $0x20
	pop     %rdx; pop %rax
	iretq

	.align 8
	.globl timer_ticks
timer_ticks: .quad 0

/* ---- yield (int $0x81) — 仅 iret ---- */
	.globl sys_yield_entry
sys_yield_entry:
	iretq

/* ---- exit (int $0x83) — 仅 iret ---- */
	.globl sys_exit_entry
sys_exit_entry:
	iretq
