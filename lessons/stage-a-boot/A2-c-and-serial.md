# Day 3: 进入 C 代码 + 串口驱动

> 对照: reference/linux-7.1/arch/x86/boot/early_serial_console.c

## 1. 目标与验收
- **目标**: 汇编 start64 调用 C 函数 start_kernel()，串口打印 "Hello"
- **验收**: QEMU 串口看到 "Hello from my kernel"

## 2. 真实源码导读
- `early_serial_console.c` — 8250 UART: COM1=0x3F8, 9600 8N1, 轮询 LSR bit5(THRE)
- `arch/x86/kernel/head_64.S:startup_64` — 清零段寄存器→设栈→调 C

## 3. 我的实现
- `head.S` — start64: xor %eax,%eax; mov %eax,%ds/es/fs/gs/ss; lea stack_top(%rip),%rsp; call start_kernel
- `serial.c` — serial_init(9600 8N1), serial_putchar(轮询THRE), serial_puts
- `kernel/main.c` — start_kernel() 调用 serial_init()→serial_puts("Hello...")

## 4. CPU 指令包装(汇编)
```asm
.globl outb; outb: movl %edi,%edx; movl %esi,%eax; outb %al,%dx; ret
.globl inb;  inb:  movl %edi,%edx; inb %dx,%al; ret
```

## 5. 运行验证
```
Hello from my kernel
```
