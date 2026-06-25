/*
 * hello_mod.c — 测试内核模块(调用内核符号版)
 */
extern void serial_puts(const char *str);

int init_module(void)
{
    serial_puts("Hello from kernel module!\r\n");
    return 0;
}

void cleanup_module(void)
{
    serial_puts("Module cleanup\r\n");
}
