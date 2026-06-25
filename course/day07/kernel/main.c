/* kernel/main.c — Day 7 */
#include "kernel.h"
#include "mm.h"

extern void gdt_init(void), idt_init(void);
extern void serial_init(void), serial_puts(const char *);
extern void pmm_init(void), buddy_init(void), slab_init(void);

void start_kernel(void) {
    serial_init();
    serial_puts("Hello from my kernel - Day 7\\n");
    gdt_init(); idt_init();
    pmm_init(); buddy_init();
    while (1) __asm__ volatile ("hlt");
}
