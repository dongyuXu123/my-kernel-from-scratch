/*
 * kernel/kdb.c — 内核调试器 (Day 59)
 *
 * 对照: reference/linux-7.1/kernel/debug/kdb/
 *
 * 串口命令行: reg, mem, bp, step, continue
 */

#include "kernel.h"

static int kdb_enabled = 1;
static unsigned long kdb_bp_addr = 0;

/* kdb_show_regs — 显示寄存器 */
static void kdb_show_regs(void)
{
    unsigned long rax, rbx, rcx, rdx, rsi, rdi, rsp, rip;
    __asm__ volatile (
        "movq %%rax, %0\n"
        "movq %%rbx, %1\n"
        "movq %%rcx, %2\n"
        "movq %%rdx, %3\n"
        "movq %%rsi, %4\n"
        "movq %%rdi, %5\n"
        "movq %%rsp, %6\n"
        : "=r"(rax), "=r"(rbx), "=r"(rcx), "=r"(rdx),
          "=r"(rsi), "=r"(rdi), "=r"(rsp)
        :
    );
    /* 获取 RIP (caller) */
    rip = (unsigned long)__builtin_return_address(0);

    serial_puts("KDB: RAX="); print_hex64(rax); serial_puts("\r\n");
    serial_puts("     RBX="); print_hex64(rbx); serial_puts("\r\n");
    serial_puts("     RCX="); print_hex64(rcx); serial_puts("\r\n");
    serial_puts("     RDX="); print_hex64(rdx); serial_puts("\r\n");
    serial_puts("     RSI="); print_hex64(rsi); serial_puts("\r\n");
    serial_puts("     RDI="); print_hex64(rdi); serial_puts("\r\n");
    serial_puts("     RSP="); print_hex64(rsp); serial_puts("\r\n");
    serial_puts("     RIP="); print_hex64(rip); serial_puts("\r\n");
}

/* kdb_mem_dump — 内存 dump */
static void kdb_mem_dump(unsigned long addr, int count)
{
    for (int i = 0; i < count; i++) {
        unsigned long val = *(unsigned long *)(addr + i * 8);
        print_hex64(addr + i * 8);
        serial_puts(": ");
        print_hex64(val);
        serial_puts("\r\n");
    }
}

/* kdb_set_bp — 设置断点 (简化: 仅记录地址) */
static void kdb_set_bp(unsigned long addr)
{
    kdb_bp_addr = addr;
    serial_puts("KDB: breakpoint set at ");
    print_hex64(addr);
    serial_puts("\r\n");
}

/* kdb_command — 处理调试命令 */
void kdb_command(const char *cmd)
{
    if (!kdb_enabled) return;

    if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'g') {
        kdb_show_regs();
    } else if (cmd[0] == 'x' || cmd[0] == 'd') {
        /* x/16x addr 或 d addr count */
        unsigned long addr = 0;
        for (const char *p = cmd + 1; *p; p++) {
            if (*p >= '0' && *p <= '9') { addr = addr * 16 + (*p - '0'); }
            else if (*p >= 'a' && *p <= 'f') { addr = addr * 16 + (*p - 'a' + 10); }
            else if (*p >= 'A' && *p <= 'F') { addr = addr * 16 + (*p - 'A' + 10); }
        }
        kdb_mem_dump(addr, 8);
    } else if (cmd[0] == 'b' && cmd[1] == 'p') {
        kdb_set_bp(0x100000);  /* 简化 */
    } else if (cmd[0] == 'c') {
        serial_puts("KDB: continuing\r\n");
    } else {
        serial_puts("KDB: unknown cmd (reg/x/bp/c)\r\n");
    }
}

/* kdb_init — 初始化调试器 */
void kdb_init(void)
{
    serial_puts("KDB: kernel debugger ready\r\n");
    serial_puts("     commands: reg, x<addr>, bp<addr>, c\r\n");
}
