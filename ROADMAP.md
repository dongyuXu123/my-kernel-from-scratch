# Linux 内核从零手写 — 60 天课程路线图

> 参照 Linux v7.1 源码，按真实启动顺序逐层构建
> 每节课对应 1 天学习量（~2小时），含源码导读 + 动手实现 + QEMU 验证

## 学习原则

1. **先读源码，再写代码** — 每节先读 reference/linux-7.1 对应文件
2. **每节可运行验证** — QEMU 串口输出即为验收
3. **汇编只做必须** — C 实现所有逻辑，汇编仅用于 CPU 指令包装
4. **渐进式构建** — 不跳步，每层依赖前一层

---

## 第 1 周：引导与早期打印（阶段 A）

### Day 1: 沙盘搭建 — QEMU + GDB + Makefile
- **目标**: 搭建编译和调试环境
- **源码**: `reference/linux-7.1/Makefile`, `arch/x86/boot/header.S`
- **文件**: `linker.ld`, `Makefile`, `arch/x86/boot.S` (最小死循环)
- **验收**: QEMU 加载不报错，gdb 命中 _start

### Day 2: 32→64 位切换 — multiboot + 长模式
- **目标**: 从 32 位保护模式切换到 64 位长模式
- **源码**: `reference/linux-7.1/arch/x86/boot/compressed/head_64.S`
- **文件**: `arch/x86/boot.S` (建临时页表 + EFER.LME + CR0.PG + ljmp)
- **验收**: gdb 确认 CS 进入 64 位，RIP 在 64 位代码段

### Day 3: 进入 C 代码 + 串口驱动
- **目标**: 汇编跳转到 C 函数，串口打印 "Hello"
- **源码**: `reference/linux-7.1/arch/x86/boot/early_serial_console.c`
- **文件**: `head.S` (start64), `serial.c` (8250 UART), `kernel/main.c`
- **验收**: QEMU 串口输出 "Hello from my kernel"

### Day 4: GDT/IDT 初始化 — C 代码配置段描述符
- **目标**: 用 C 代码填充 GDT/IDT，触发 int3 验证中断
- **源码**: `reference/linux-7.1/arch/x86/kernel/idt.c`
- **文件**: `gdt.c`, `idt.c`, `entry.S` (int3 handler)
- **验收**: `int3` 触发后打印 "3"，然后返回继续执行

### Day 5: 异常处理 #PF/#GP/#DF
- **目标**: 实现三个关键异常处理器
- **源码**: `reference/linux-7.1/arch/x86/mm/fault.c`
- **文件**: `entry.S` (pf/gp/df handler)
- **验收**: 访问 0xDEAD0000 触发 #PF，打印 CR2 后返回

---

## 第 2 周：内存管理（阶段 B）

### Day 6: 物理内存 Bitmap 分配器
- **目标**: 实现最简物理页分配/释放
- **源码**: `reference/linux-7.1/mm/memblock.c`
- **文件**: `mm/pmm.c`, `include/mm.h`
- **验收**: alloc_page 返回合理物理地址

### Day 7: Buddy System 伙伴分配器
- **目标**: 实现 2^N 伙伴分配/合并
- **源码**: `reference/linux-7.1/mm/page_alloc.c`
- **文件**: `mm/buddy.c`
- **验收**: alloc→free→alloc 返回同一页

### Day 8: Slab 分配器 (kmalloc/kfree)
- **目标**: 固定 64 字节对象分配器
- **源码**: `reference/linux-7.1/mm/slub.c`
- **文件**: `mm/slab.c`
- **验收**: kmalloc(64) 返回有效内存

### Day 9: 4 级页表初始化
- **目标**: 将 2MB 大页切换为 4KB 页表，设置 U/S=1
- **源码**: `reference/linux-7.1/arch/x86/mm/init.c`
- **文件**: `mm/page.c`
- **验收**: setup_pagetables 后代码仍正常运行

### Day 10: 页表映射扩展 (map_mmio)
- **目标**: 动态添加 2MB 大页映射任意物理地址
- **源码**: `reference/linux-7.1/arch/x86/mm/init_64.c`
- **文件**: `mm/page.c` (map_mmio)
- **验收**: 跨 PDP 索引映射 PCI MMIO 地址成功

---

## 第 3 周：进程与系统调用（阶段 C）

### Day 11: 任务结构 + 调度器初始化
- **目标**: 定义 task_struct，初始化任务链表
- **源码**: `reference/linux-7.1/include/linux/sched.h`
- **文件**: `include/sched.h`, `kernel/sched.c`, `kernel/task.c`
- **验收**: gdb 看到 tasks[0]/tasks[1] 正确链接

### Day 12: 上下文切换 (switch_to)
- **目标**: 汇编实现任务栈切换
- **源码**: `reference/linux-7.1/arch/x86/entry/entry_64.S`
- **文件**: `arch/x86/switch.S`
- **验收**: 两个任务交替打印

### Day 13: TSS 与用户态切换 (ring3)
- **目标**: 设置 TSS.RSP0，iretq 切换到 ring3
- **源码**: `reference/linux-7.1/arch/x86/kernel/process_64.c`
- **文件**: `arch/x86/tss.c`, `head.S` (enter_user_mode_asm)
- **验收**: ring3 代码在串口打印字符

### Day 14: syscall 机制 (int $0x80)
- **目标**: 通过 IDT 注册 DPL=3 中断门，实现系统调用分派
- **源码**: `reference/linux-7.1/arch/x86/entry/entry_64.S`
- **文件**: `idt.c`, `entry.S` (syscall dispatch)
- **验收**: ring3 调用 write 在串口打印

### Day 15: fork + yield (协作式调度)
- **目标**: 复制内核栈创建子进程，yield 切换
- **源码**: `reference/linux-7.1/kernel/fork.c`
- **文件**: `entry.S` (fork + sys_yield_entry)
- **验收**: fork 后父子交替打印 P/C

---

## 第 4 周：文件系统（阶段 D）

### Day 16: C 编译框架 + ramfs 设计
- **目标**: 多文件 C+ASM 混合编译，设计 ramfs 数据结构
- **源码**: `reference/linux-7.1/fs/ramfs/inode.c`
- **文件**: `Makefile`, `include/kernel.h`, `fs/ramfs.c`
- **验收**: ramfs_init/create/write 编译通过

### Day 17: ramfs 实现 (create/write/read/list)
- **目标**: 16 文件内存文件系统完整实现
- **源码**: `reference/linux-7.1/fs/ramfs/`
- **文件**: `fs/ramfs.c`
- **验收**: ring3 调用 open→write→list 工作

### Day 18: ELF 加载器
- **目标**: 解析 ELF64 header，复制 PT_LOAD 段
- **源码**: `reference/linux-7.1/fs/binfmt_elf.c`
- **文件**: `fs/elf.c`
- **验收**: 嵌入的 /init ELF 被加载执行

### Day 19: exec 系统调用
- **目标**: 替换当前进程为新 ELF 程序
- **源码**: `reference/linux-7.1/fs/exec.c`
- **文件**: `fs/exec.c`, `entry.S` (.do_exec)
- **验收**: `exec hello` 运行 /bin/hello

### Day 20: 交互 Shell (/init)
- **目标**: 用户态程序实现命令解析 + 回显
- **文件**: `initfs/init.S`
- **验收**: `# ` 提示符，help/echo/ls/cat 命令

---

## 第 5 周：驱动与设备模型（阶段 E）

### Day 21: 块设备层 (bio + RAM 盘)
- **目标**: 实现 bio 分配/提交/完成，brd 驱动
- **源码**: `reference/linux-7.1/block/bio.c`, `drivers/block/brd.c`
- **文件**: `include/blkdev.h`, `block/bio.c`, `block/brd.c`
- **验收**: blkdev write→read 数据一致

### Day 22: ext2 只读文件系统
- **目标**: 解析 superblock/inode/dir/file
- **源码**: `reference/linux-7.1/fs/ext2/`
- **文件**: `fs/ext2.c`
- **验收**: 读取 "Hello ext2!" 内容

### Day 23: 模块加载器
- **目标**: 解析 ET_REL ELF，符号解析 + 重定位
- **源码**: `reference/linux-7.1/kernel/module/main.c`
- **文件**: `kernel/module.c`
- **验收**: insmod 打印 "Hello from asm module!"

### Day 24: PCI 枚举 + e1000 网卡驱动
- **目标**: 扫描 PCI 总线，初始化 e1000 发送以太网帧
- **源码**: `reference/linux-7.1/drivers/pci/`, `e1000`
- **文件**: `arch/x86/pci.c`, `arch/x86/e1000.c`
- **验收**: e1000 send OK

### Day 25: 定时器中断 (PIT + PIC)
- **目标**: 初始化 8253 PIT，重映射 8259 PIC，处理 IRQ0
- **源码**: `reference/linux-7.1/drivers/clocksource/i8253.c`
- **文件**: `arch/x86/pic.c`, `arch/x86/pit.c`, `entry.S` (timer_handler)
- **验收**: 定时器中断触发，打印 ticks

---

## 第 6 周：设备模型与网络

### Day 26: 设备驱动模型 (bus/driver/device)
- **目标**: 实现总线注册、设备注册、驱动匹配 + probe
- **源码**: `reference/linux-7.1/drivers/base/`
- **文件**: `include/device.h`, `kernel/device.c`
- **验收**: PCI 总线 → e1000 驱动匹配 → probe 调用

### Day 27: 抢占式多任务调度
- **目标**: 定时器中断中切换 current_task，时间片轮转
- **源码**: `reference/linux-7.1/kernel/sched/core.c`
- **文件**: `entry.S` (timer_handler 切换)
- **验收**: worker 任务与 /init 交替运行

### Day 28: e1000 RX + ARP/ICMP 协议栈
- **目标**: 配置 RX 环，ARP 应答 + ICMP Echo Reply
- **源码**: `reference/linux-7.1/net/ipv4/arp.c`
- **文件**: `arch/x86/e1000.c` (RX), `net/net.c`
- **验收**: ARP/ICMP 处理器正确格式化响应包

### Day 29: VFS 抽象层 + 文件描述符
- **目标**: 定义 file/inode/file_operations，统一 fd 表
- **源码**: `reference/linux-7.1/include/linux/fs.h`
- **文件**: `include/vfs.h`, `fs/vfs.c`, `kernel/fd.c`
- **验收**: open→read→write 通过 VFS 操作 ramfs

### Day 30: 集成测试 + 课程总结
- **目标**: 全功能回归测试，输出系统能力清单
- **验收**: 引导→内存→进程→Shell→PCI→e1000→模块→定时器→VFS 全部通过

---

## 第 7 周：GUI 图形界面（阶段 F）

### Day 31: PS/2 鼠标驱动
- **目标**: 初始化 PS/2 鼠标，解析 3 字节数据包，framebuffer 光标绘制
- **参考**: tinywm (github.com/mackstann/tinywm), OSDev PS/2 Mouse
- **文件**: `arch/x86/mouse.c`, `arch/x86/fb.c`
- **验收**: 移动鼠标可见白色箭头光标跟随

### Day 32: 窗口管理器
- **目标**: 窗口抽象(位置/大小/标题)，fill_rect 绘制，标题栏拖拽移动
- **参考**: tinywm 的 move/resize/raise 概念, X11 Window Manager
- **文件**: `arch/x86/wm.c`
- **验收**: 拖动窗口标题栏，窗口跟随鼠标移动

### Day 33: GUI 控件 (Button/Label/Input)
- **目标**: 3D 边框效果，点击回调节器，键盘输入绑定
- **参考**: Windows GDI / GTK/Qt Widget 模型
- **文件**: `arch/x86/widget.c`
- **验收**: 点击按钮触发回调，文本框接受键盘输入

### Day 34: 桌面环境 (Taskbar + Clock)
- **目标**: 底部任务栏，PIT 驱动时钟(时:分:秒)，应用启动器按钮
- **参考**: Desktop Metaphor (Xerox PARC), Windows/Mac/Linux DE
- **文件**: `arch/x86/desktop.c`
- **验收**: 任务栏显示时钟计时 + 启动器按钮

### Day 35: GUI 应用程序 (Calculator/Editor/Browser)
- **目标**: 简易计算器，文本编辑器，ramfs 文件浏览器
- **参考**: MVC Pattern, Windows Notepad/Calculator
- **文件**: `arch/x86/apps.c`
- **验收**: 完整桌面环境: 应用启动器/窗口/控件/时钟

---

## 第 8 周：高级特性（阶段 G）

### Day 36: 管道 (Pipe)
- **目标**: pipe() + dup2() 系统调用，环形缓冲区，shell 管道解析
- **源码**: `reference/linux-7.1/fs/pipe.c`
- **文件**: `kernel/fd.c`, `arch/x86/entry.S`
- **验收**: `ls | grep hello` 管道工作

### Day 37: 信号 (Signal)
- **目标**: signal() + kill() 系统调用，Ctrl-C → SIGINT 处理
- **源码**: `reference/linux-7.1/kernel/signal.c`
- **文件**: `kernel/signal.c`, `arch/x86/entry.S`
- **验收**: Ctrl-C 触发信号处理函数

### Day 38: TCP 协议栈
- **目标**: TCP 首部结构，SYN 包发送，理解三次握手
- **源码**: `reference/linux-7.1/net/ipv4/tcp.c`
- **文件**: `net/tcp.c`
- **验收**: SYN 包发送，Wireshark 可抓包

### Day 39: HTTP 协议
- **目标**: HTTP/1.1 GET 请求构建，应用层协议栈分层
- **源码**: `reference/linux-7.1/net/`
- **文件**: `net/http.c`
- **验收**: GET 请求通过 TCP 发送

### Day 40: Framebuffer 控制台
- **目标**: PCI VGA 探测，8x16 位图字体，fb_putchar/fb_puts
- **文件**: `arch/x86/fb.c`, `mm/page.c` (map_mmio)
- **验收**: Framebuffer 终端输出

---

## 第 9 周：附加主题（阶段 H）

### Day 41: 多地址空间 (Per-Process Page Tables)
- **目标**: 每进程独立 PML4，fork 复制页表，CR3 上下文切换，独立内核栈
- **源码**: `reference/linux-7.1/kernel/fork.c (dup_mm)`, `arch/x86/kernel/process_64.c (__switch_to)`
- **文件**: `mm/page.c (clone_kernel_pml4)`, `kernel/task.c`, `arch/x86/entry.S`
- **验收**: fork 后父子进程有独立的 cr3 和内核栈

### Day 42: 抢占式多任务调度
- **目标**: 定时器中断中调用 schedule()，每进程独立内核栈，CR3+TSS.RSP0 更新
- **源码**: `reference/linux-7.1/kernel/sched/core.c (scheduler_tick)`
- **文件**: `arch/x86/entry.S (timer_handler)`, `arch/x86/tss.c`
- **验收**: 定时器驱动进程切换 (100Hz → 每 8 ticks 调度)

### Day 43: AHCI SATA 磁盘驱动
- **目标**: PCI class 扫描，AHCI HBA 初始化，FIS DMA 读写扇区，注册为块设备
- **源码**: `reference/linux-7.1/drivers/ata/ahci.c`
- **文件**: `arch/x86/ahci.c`, `arch/x86/pci.c`, `include/device.h`
- **验收**: AHCI 磁盘识别并注册为 /dev/sda

### Day 44: ext2 写入支持
- **目标**: inode/block 分配器，ext2_create 创建文件，ext2_write_file 写数据
- **源码**: `reference/linux-7.1/fs/ext2/ialloc.c, balloc.c, inode.c`
- **文件**: `fs/ext2.c`
- **验收**: 在 ext2 文件系统创建并写入文件

### Day 45: BusyBox 移植准备 (Syscall 扩展)
- **目标**: 新增 close(15)/lseek(16)/fstat(17)/getpid(18)/waitpid(19)/brk(20)/exit(60)，修复 open 接受文件名
- **源码**: `reference/linux-7.1/fs/file.c, fs/read_write.c, fs/stat.c`
- **文件**: `arch/x86/entry.S`, `kernel/fd.c`, `fs/ramfs.c`
- **验收**: 新 syscall 可通过测试程序验证

---

## 第 10 周：深度扩展（阶段 I）

### Day 46: Copy-on-Write Fork
- **目标**: fork 后父子共享只读页，#PF 触发 COW 复制
- **源码**: `reference/linux-7.1/mm/memory.c (copy_page_range, do_wp_page)`
- **文件**: `mm/page.c`, `arch/x86/entry.S`
- **验收**: fork 后写用户页触发 COW

### Day 47: ELF PIE 支持
- **目标**: ET_DYN 类型支持，EXEC_BUF_SIZE 256KB
- **源码**: `reference/linux-7.1/fs/binfmt_elf.c`
- **文件**: `fs/elf.c`, `fs/exec.c`
- **验收**: 可加载 PIE 编译的二进制

### Day 48: TCP 完整三次握手
- **目标**: TCP 状态机 + SYN→SYN-ACK→ACK
- **源码**: `reference/linux-7.1/net/ipv4/tcp_input.c`
- **文件**: `net/tcp.c`
- **验收**: 完整三次握手，可用 Wireshark 验证

### Day 49: HTTP 服务器
- **目标**: TCP 之上实现 HTTP/1.1，解析 GET 返回 HTML
- **源码**: (用户态协议，参考 RFC 7230)
- **文件**: `net/http.c`
- **验收**: curl 可获取页面

### Day 50: ext2 间接块 + 符号链接 + 删除
- **目标**: 一级间接块、快速符号链接、文件删除
- **源码**: `reference/linux-7.1/fs/ext2/inode.c`
- **文件**: `fs/ext2.c`
- **验收**: 支持 >12KB 文件和 symlink

---

## 第 11 周：新领域（阶段 J）

### Day 51: UDP + DHCP
- **目标**: UDP 收发 + DHCP 自动获取 IP
- **文件**: `net/udp.c`, `net/dhcp.c` (新)

### Day 52: /proc 伪文件系统
- **目标**: /proc/meminfo, /proc/cpuinfo, /proc/self
- **文件**: `fs/procfs.c` (新)

### Day 53: SMP 多核启动
- **目标**: APIC + IPI + BSP 启动 AP
- **文件**: `arch/x86/smp.c`, `arch/x86/apic.c` (新)

### Day 54: 自旋锁 + 并行调度
- **目标**: spinlock + per-CPU runqueue + 负载均衡
- **文件**: `kernel/spinlock.c` (新), `kernel/sched.c`

### Day 55: RCU 同步机制
- **目标**: Read-Copy-Update + 宽限期 + 链表操作
- **文件**: `kernel/rcu.c` (新)

---

## 第 12 周：生态 + 收尾（阶段 K）

### Day 56: USB 驱动
- **目标**: UHCI/EHCI + 设备枚举 + HID 键盘
- **文件**: `arch/x86/usb.c` (新)

### Day 57: 用户态 C 程序 (newlib)
- **目标**: 交叉编译 newlib + C 程序在自研内核运行
- **文件**: `initfs/crt0.S`, `initfs/syscalls.c` (新)

### Day 58: 微内核 IPC 实验
- **目标**: send/recv/reply 消息传递
- **文件**: `kernel/ipc.c` (新)

### Day 59: 内核调试器 (KDB)
- **目标**: 串口命令调试器 (reg/mem/bp/step)
- **文件**: `kernel/kdb.c` (新)

### Day 60: 最终集成 + 性能基准
- **目标**: 全功能回归 + 性能测试 + 课程总结
- **验收**: 60 天全部特性通过

---

## 文件索引

| 阶段 | 目录 | 关键文件 |
|------|------|---------|
| A | arch/x86/ | boot.S, head.S, serial.c, gdt.c, idt.c, tss.c |
| B | mm/ | pmm.c, buddy.c, slab.c, page.c |
| C | kernel/ | sched.c, task.c, switch.S, entry.S |
| D | fs/ | ramfs.c, elf.c, exec.c, ext2.c |
| E | arch/x86/ + kernel/ + net/ | pci.c, e1000.c, pic.c, pit.c, module.c, fd.c, device.c, vfs.c, net.c |
| F (GUI) | arch/x86/ | mouse.c, wm.c, widget.c, desktop.c, apps.c, fb.c |
| G (Adv) | kernel/ + net/ + arch/x86/ | fd.c (pipe), signal.c, tcp.c, http.c, fb.c |
| H (Ext) | arch/x86/ + kernel/ + fs/ | ahci.c, pci.c (class), ext2.c (write), fd.c (close/lseek) |
| I (Deep) | mm/ + fs/ + net/ | page.c (COW), elf.c (ET_DYN), tcp.c (state machine), ext2.c (indirect) |
| J (New) | kernel/ + net/ + arch/x86/ | udp.c, dhcp.c, procfs.c, smp.c, apic.c, spinlock.c, rcu.c |
| K (Eco) | kernel/ + fs/ + arch/x86/ | usb.c, crt0.S, syscalls.c, ipc.c, kdb.c |

**总计**: ~85 个源文件，~10000 行代码
