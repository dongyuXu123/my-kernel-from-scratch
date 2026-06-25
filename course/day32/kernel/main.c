/* kernel/main.c — Day 32 */
#include "kernel.h"
#include "mm.h"
extern void gdt_init(void), idt_init(void);
extern void serial_init(void), serial_puts(const char *);
extern void pmm_init(void), buddy_init(void), slab_init(void);
extern void setup_pagetables(void), cow_setup(void);
extern void sched_init(void), tss_init(void);

void start_kernel(void) {
    serial_init();
    serial_puts("Hello from my kernel - Day 32\n");
    gdt_init(); idt_init();
    pmm_init(); buddy_init(); slab_init();
    setup_pagetables(); cow_setup();
    sched_init();
    tss_init();
    extern unsigned long elf_load_mem(void *, unsigned int);
    extern unsigned char init_elf_start[], init_elf_end[];
    unsigned long entry = elf_load_mem(init_elf_start, (unsigned int)(init_elf_end - init_elf_start));
    extern void enter_user_mode_asm(void *);
    if (entry) enter_user_mode_asm((void *)entry);
    while (1) __asm__ volatile ("hlt");
}
