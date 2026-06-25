/*
 * kernel/main.c — Day 22 内核初始化
 *
 * 当天新增功能: all
 */
#include "kernel.h"
#include "mm.h"

extern void gdt_init(void), idt_init(void), tss_init(void);
extern void serial_init(void), serial_puts(const char *);
extern void pmm_init(void), buddy_init(void), slab_init(void);
extern void setup_pagetables(void);
extern void sched_init(void);
extern void pic_remap(void), pit_init(unsigned int), pic_enable_irq(int);
extern void pci_scan(void);
extern void e1000_init(void);
extern void ramfs_init(void);
extern int ramfs_create(const char *), ramfs_write(int, const char *, int);
extern void fb_init(void);
extern void cow_setup(void);
extern void enter_user_mode_asm(void *);

void start_kernel(void)
{
    serial_init();
    serial_puts("Hello from my kernel - Day 22\r\n");

    gdt_init(); idt_init();
    /* 加载 /init 进入用户态 */
    extern unsigned char init_elf_start[], init_elf_end[];
    extern unsigned long elf_load_mem(void *, unsigned int);
    unsigned long entry = elf_load_mem(init_elf_start, (unsigned int)(init_elf_end - init_elf_start));
    if (entry) enter_user_mode_asm((void *)entry);
    while (1) __asm__ volatile ("hlt");
}
