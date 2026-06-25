/*
 * arch/x86/pit.c — 8253/8254 PIT 定时器
 *
 * 对照: reference/linux-7.1/drivers/clocksource/i8253.c
 *
 * 端口: 0x40=通道0数据, 0x43=命令
 * 频率: 1193182 Hz / divisor
 * 100Hz → divisor = 11932
 */

#include "kernel.h"

void pit_init(unsigned int hz)
{
    unsigned int divisor = 1193182 / hz;

    /* 通道 0, 方波模式, 二进制 */
    outb(0x43, 0x36);

    /* 低字节 + 高字节 */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    serial_puts("PIT: ");
    print_hex64(hz);
    serial_puts("Hz divisor=");
    print_hex64(divisor);
    serial_puts("\r\n");
}
