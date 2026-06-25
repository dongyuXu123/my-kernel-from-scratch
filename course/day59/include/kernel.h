/*
 * include/kernel.h — 公共类型 + 内核基础声明
 *
 * 对照: reference/linux-7.1/include/linux/kernel.h
 */

#ifndef _KERNEL_H
#define _KERNEL_H

/* -------- 基础类型 -------- */
typedef unsigned long size_t;
typedef long ssize_t;

/* -------- I/O 端口(汇编实现, head.S) -------- */
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char value);
extern unsigned int  inl(unsigned short port);
extern void outl(unsigned short port, unsigned int value);

/* -------- CPU 控制(汇编实现, head.S) -------- */
extern void lgdt_wrapper(void *gdt_ptr);
extern void lidt_wrapper(void *idt_ptr);
extern void ltr_wrapper(unsigned short selector);
extern void write_cr3(unsigned long val);
extern unsigned long read_cr2(void);
extern void write_msr(unsigned int msr, unsigned long val);
extern unsigned long read_msr(unsigned int msr);
extern void enter_user_mode_asm(void *user_rip);

/* -------- 串口输出 -------- */
extern void serial_init(void);
extern void serial_putchar(char c);
extern void serial_puts(const char *str);
extern void print_hex64(unsigned long val);

/* -------- start_kernel(定义在 kernel/main.c) -------- */
extern void start_kernel(void);

/* -------- 内核断言 -------- */
#define BUG() do { serial_puts("BUG!\r\n"); while(1); } while(0)

#endif /* _KERNEL_H */
