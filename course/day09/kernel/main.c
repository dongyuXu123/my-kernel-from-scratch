/*
 * kernel/main.c — Day 9: 4级页表
 *
 * 对照: reference/linux-7.1/init/main.c (start_kernel)
 *
 * 当天新增: 4级页表
 */
#include "kernel.h"
#include "mm.h"

extern void gdt_init(void), idt_init(void), tss_init(void);
extern void serial_init(void), serial_puts(const char *);
extern void pmm_init(void), buddy_init(void), slab_init(void);
extern void setup_pagetables(void), cow_setup(void);
extern void sched_init(void);
extern void pic_remap(void), pit_init(unsigned int), pic_enable_irq(int);
extern void pci_scan(void), e1000_init(void);
extern void ramfs_init(void);
extern int  ramfs_create(const char *), ramfs_write(int, const char *, int);
extern void enter_user_mode_asm(void *);

void start_kernel(void)
{
    serial_init();
    serial_puts("Hello from my kernel - Day 9\r\n");

    gdt_init(); idt_init();
    pmm_init(); buddy_init(); slab_init();
    setup_pagetables();
    cow_setup();
    while (1) __asm__ volatile ("hlt");
}
