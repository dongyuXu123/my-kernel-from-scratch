/* kernel/main.c — Day 3 */
#include "kernel.h"

extern void gdt_init(void), idt_init(void);
extern void serial_init(void), serial_puts(const char *);

void start_kernel(void) {
    serial_init();
    serial_puts("Hello from my kernel - Day 3\\n");
    gdt_init(); idt_init();
    while (1) __asm__ volatile ("hlt");
}
