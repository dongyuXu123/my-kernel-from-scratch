/*
 * kernel/main.c — Day 16 内核初始化
 *
 * 对照: reference/linux-7.1/init/main.c (start_kernel)
 */
#include "kernel.h"
#include "mm.h"
#include "sched.h"
#include "module.h"

extern void gdt_init(void), idt_init(void), tss_init(void);
extern void ramfs_init(void);
extern int  ramfs_create(const char *name);
extern int  ramfs_write(int fd, const char *buf, int len);
extern void enter_user_mode_asm(void *);
extern void fb_init(void), fb_puts(const char *);

int gui_mode = 0;

void start_kernel(void)
{
    serial_init();
    serial_puts("Hello from my kernel - Day 16\n");

    gdt_init(); idt_init();
    pmm_init(); buddy_init(); slab_init(); setup_pagetables();
    extern void cow_setup(void);
    cow_setup();
    sched_init(); tss_init();
    register_kernel_symbol("serial_puts", serial_puts);
    register_kernel_symbol("print_hex64", print_hex64);
    ramfs_init();
    ramfs_create("hello.txt");
    ramfs_write(0, "Hello from ramfs!", 17);
    extern unsigned char init_elf_start[], init_elf_end[];
    extern unsigned long elf_load_mem(void *, unsigned int);
    unsigned long entry = elf_load_mem(init_elf_start,
                        (unsigned int)(init_elf_end - init_elf_start));
    if (entry) enter_user_mode_asm((void *)entry);
    while (1) { __asm__ volatile ("hlt"); }
}
