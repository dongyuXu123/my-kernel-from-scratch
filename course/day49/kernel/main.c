/*
 * kernel/main.c — Day 49 内核初始化
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
    serial_puts("Hello from my kernel - Day 49\n");

    gdt_init(); idt_init();
    pmm_init(); buddy_init(); slab_init(); setup_pagetables();
    extern void cow_setup(void);
    cow_setup();
    sched_init(); tss_init();
    extern void pic_remap(void), pit_init(unsigned int), pic_enable_irq(int);
    pic_remap(); pit_init(100); pic_enable_irq(0);
    extern void pci_scan(void), e1000_init(void);
    pci_scan(); e1000_init();
    register_kernel_symbol("serial_puts", serial_puts);
    register_kernel_symbol("print_hex64", print_hex64);
    ramfs_init();
    ramfs_create("hello.txt");
    ramfs_write(0, "Hello from ramfs!", 17);
    extern void tcp_listen(void);
    tcp_listen();
    extern void smp_init(void), apic_init(void);
    apic_init(); smp_init();
    extern unsigned int *fb;
    int console_boot = 1;
    fb_init();
    extern unsigned int *fb;
    if (!console_boot && fb) {
        gui_mode = 1;
        extern void mouse_init(void), wm_init(void);
        mouse_init(); wm_init();
        extern void desktop_init(void), apps_init(void);
        desktop_init(); apps_init();
        extern int wm_create_window(int,int,int,int,const char*);
        wm_create_window(50, 50, 320, 200, "Day 49");
        while (1) { extern void mouse_poll(void), mouse_draw(void);
            mouse_poll(); mouse_draw(); __asm__("hlt"); }
    }
    serial_puts("No framebuffer, starting console...\r\n");
    extern unsigned char init_elf_start[], init_elf_end[];
    extern unsigned long elf_load_mem(void *, unsigned int);
    unsigned long entry = elf_load_mem(init_elf_start,
                        (unsigned int)(init_elf_end - init_elf_start));
    if (entry) enter_user_mode_asm((void *)entry);
    while (1) { __asm__ volatile ("hlt"); }
}
