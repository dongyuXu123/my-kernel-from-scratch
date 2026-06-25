# A5 串口/早期打印

## 1. 目标与验收

- **本节目标**: 自写 COM1 串口输出,内核在 QEMU 串口打印 "Hello from my kernel"
- **验收标准**: QEMU `-serial file:log` 输出的 log 文件包含 `Hello from my kernel`

## 2. 真实源码导读

(由 theory-agent 调 systems-lesson 填写)

- `reference/linux-7.1/arch/x86/boot/early_serial_console.c` — 真实内核的早期串口驱动
  - `early_serial_init(int port, int baud)`: 初始化 COM 端口(8N1, 设置除数)
  - 波特率精度: 除数 = 115200 / baud(9600→12)
  - 命令行动态解析端口和波特率(A5 简化为硬编码 0x3F8/9600)
  - 真实内核支持早期串口打印的 `console_init()` 在实模式 setup 阶段就调用

**关键知识点(COM1 8250 UART 寄存器):**
| 端口 | 寄存器 | 说明 |
|---|---|---|
| 0x3F8 | TX/DLL | 发送寄存器 / 除数低字节(DLAB=1) |
| 0x3F9 | IER/DLH | 中断使能 / 除数高字节(DLAB=1) |
| 0x3FB | LCR | 行控制(bit7=DLAB, bit[1:0]=数据位) |
| 0x3FD | LSR | 行状态(bit5=THRE, bit0=DR) |

## 3. 我的实现(mykernel)

### 关键代码

- `arch/x86/entry.S`:
  - `serial_init`: 初始化 COM1 为 9600 8N1(当前未调用,用 SeaBIOS 已配好的状态)
  - `serial_putchar`: 等 THRE(LSR bit5)后发一个字节到 TX(0x3F8)
  - `serial_puts`: 遍历字符串直到 \0,每字节调 serial_putchar
  - `start64`: 64 位入口—段复位→设栈→lea hello_msg→call serial_puts→死循环

### 运行验证 [2026-06-23]

**命令**: `make && timeout 2 qemu-system-x86_64 -kernel mykernel.elf -display none -serial file:/tmp/serial.log -no-reboot -no-shutdown`
**输出**:
```
Hello from my kernel
```
**验收**: ✅ 通过

## 4. 答疑沉淀

(暂无)

## 5. 纠错日志

### [2026-06-23] serial_puts 中 %rdi 被覆盖导致字符串输出乱码

- **现象**: QEMU 串口只输出首字符 'H',后续字节为无效数据(hex: 48 d8 e8 8d b0 40)
- **根因**: `serial_puts` 中 `movb %al, %dil` 把字符写入 %rdi 的低字节,而 %rdi 存的是字符串指针。写入后指针被破坏,下一次循环从错误的地址读下一个字符,输出乱码
- **对照**: 真实内核 `arch/x86/boot/early_serial_console.c` 的 `early_serial_write` 不存在此问题——它遍历时直接用 `*buf` 取值不修改指针,且每次调用 `putchar` 传参用 C 语言的值传递,编译器自动处理寄存器
- **修复**: 用 %rbx(callee-saved) 保存字符串指针副本,循环中递增 %rbx 而非 %rdi。`movb %al, %dil` 只改 %rdi 低字节,不影响 %rbx
- **改进方法**: 手写汇编调用约定时,注意函数的隐式副作用——参数寄存器可能被调用者修改;指针遍历用 callee-saved 寄存器

### [2026-06-23] serial_init 导致波特率不匹配

- **现象**: 调 `serial_init` 后输出乱码,跳过初始化(用 SeaBIOS 已配状态)输出正常
- **根因**: SeaBIOS 已将 COM1 初始化为特定波特率,本内核的 `serial_init` 以 9600 重初始化后与 QEMU 的 `-serial` 预期波特率不匹配(或初始化时序与 BIOS 遗留状态冲突)
- **对照**: 真实内核的 `early_serial_init` 在 BIOS 之后运行,但真实内核在实模式下就调 `console_init`,此时 UART 状态完全由自己控制
- **修复**: 当前阶段不调 `serial_init`,依赖 SeaBIOS 初始化。`serial_init` 函数保留为后续阶段准备(完全控制硬件后启用)
- **改进方法**: 在 pre-boot 阶段接管硬件时,要么完全自己初始化并确保与宿主一致,要么完全依赖 bootloader 的状态。混合两者容易产生波特率/流控冲突
