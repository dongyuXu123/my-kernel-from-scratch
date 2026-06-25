/*
 * arch/x86/serial.c — 8250 UART 串口驱动
 *
 * 对照: reference/linux-7.1/arch/x86/boot/early_serial_console.c
 *
 * 寄存器(COM1):
 *   0x3F8 = TX/RX (DLAB=0), 除数低(DLAB=1)
 *   0x3F9 = IER (DLAB=0), 除数高(DLAB=1)
 *   0x3FB = LCR (bit7=DLAB)
 *   0x3FD = LSR (bit5=THRE 发送就绪)
 */

#include "kernel.h"

/* ================================================================
 * serial_init — 初始化 COM1 为 9600 8N1
 * ================================================================ */
void serial_init(void)
{
    /* 关中断 */
    outb(0x3F9, 0x00);

    /* DLAB=1, 设波特率 */
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x0C);   /* 除数低 = 115200/9600 = 12 */
    outb(0x3F9, 0x00);   /* 除数高 = 0 */

    /* 8N1, DLAB=0 */
    outb(0x3FB, 0x03);
}

/* ================================================================
 * serial_putchar — 输出一个字符(轮询等 THRE)
 * ================================================================ */
void serial_putchar(char c)
{
    /* 等发送缓冲空(THRE=1) */
    while ((inb(0x3FD) & 0x20) == 0)
        ;
    outb(0x3F8, c);
}

/* ================================================================
 * serial_puts — 输出字符串(null-terminated)
 * ================================================================ */
void serial_puts(const char *str)
{
    while (*str) {
        serial_putchar(*str++);
    }
}

/* ================================================================
 * print_hex64 — 打印 64 位十六进制数
 * ================================================================ */
void print_hex64(unsigned long val)
{
    for (int i = 0; i < 16; i++) {
        unsigned long nibble = (val >> (60 - i * 4)) & 0xF;
        char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        serial_putchar(c);
    }
}
