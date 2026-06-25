# 内核课学习进度

> 60 天课程 (全部完成)，每天 ~2 小时。详细笔记见 `lessons/<阶段>/<课节>.md`
> 对照真实源码: Linux v7.1

## 第 1 周：引导与早期打印（阶段 A）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 1 | A0 | QEMU+GDB+Makefile | linker.ld, boot.S | QEMU 加载不报错 |
| 2 | A1 | 32→64 位长模式 | boot.S (EFER.LME+CR0.PG+ljmp) | gdb 确认 64 位 CS |
| 3 | A2 | 进入 C + 串口 | head.S, serial.c, main.c | "Hello from my kernel" |
| 4 | A3 | GDT/IDT 初始化 | gdt.c, idt.c, entry.S | int3→打印'3'→返回 |
| 5 | A4 | #PF/#GP/#DF | entry.S (handler) | #PF 打印 CR2 |

## 第 2 周：内存管理（阶段 B）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 6 | B1 | Bitmap 分配器 | mm/pmm.c | alloc_page 返回物理地址 |
| 7 | B2 | Buddy System | mm/buddy.c | alloc→free→alloc 同页 |
| 8 | B3 | Slab(kmalloc/kfree) | mm/slab.c | kmalloc(64) 有效 |
| 9 | B4 | 4 级页表 | mm/page.c | U/S=1 后代码正常运行 |
| 10 | B5 | map_mmio | mm/page.c | 跨 PDP 映射 PCI MMIO |

## 第 3 周：进程与系统调用（阶段 C）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 11 | C1 | task_struct+调度器 | sched.c, task.c | tasks 链表正确 |
| 12 | C2 | switch_to | switch.S | 交替打印 |
| 13 | C3 | TSS+ring3 | tss.c, head.S | ring3 Direct I/O |
| 14 | C4 | syscall(int $0x80) | idt.c, entry.S | ring3 write→串口 |
| 15 | C5 | fork+yield | entry.S (fork/yield) | 父子交替 PCPC |

## 第 4 周：文件系统（阶段 D）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 16 | D1 | C 编译框架+ramfs | Makefile, kernel.h, ramfs.c | 编译通过 |
| 17 | D2 | ramfs 完整实现 | ramfs.c | open→write→list |
| 18 | D3 | ELF 加载器 | elf.c | /init 被加载执行 |
| 19 | D4 | exec | exec.c, entry.S | exec hello 成功 |
| 20 | D5 | 交互 Shell | initfs/init.S | # help/echo/ls/cat |

## 第 5 周：驱动与设备模型（阶段 E）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 21 | E1 | 块设备层 | blkdev.h, bio.c, brd.c | write→read 一致 |
| 22 | E2 | ext2 只读 | ext2.c | 读 "Hello ext2!" |
| 23 | E3 | 模块加载器 | module.c | insmod 打印成功 |
| 24 | E4 | PCI+e1000 | pci.c, e1000.c | e1000 send OK |
| 25 | E5 | 定时器(PIT+PIC) | pic.c, pit.c, entry.S | 中断触发 |

## 第 6 周：设备模型与网络（阶段 E 续）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 26 | E6 | 设备模型 | device.h, device.c | bus→match→probe |
| 27 | E7 | 抢占调度 | entry.S (timer切换) | WWWW 交替 |
| 28 | E8 | e1000 RX+ARP/ICMP | e1000.c, net.c | 响应包正确构建 |
| 29 | E9 | VFS+fd | vfs.h, vfs.c, fd.c | open→read→write |
| 30 | E10 | 集成测试 | 全部文件 | 全功能通过 |

## 第 7 周：GUI 图形界面（阶段 F）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 31 | F1 | PS/2 鼠标驱动 | mouse.c, fb.c | 光标跟随移动 |
| 32 | F2 | 窗口管理器 | wm.c | 拖拽移动窗口 |
| 33 | F3 | GUI 控件 | widget.c | 按钮/标签/文本框 |
| 34 | F4 | 桌面环境 | desktop.c | 任务栏+时钟 |
| 35 | F5 | GUI 应用程序 | apps.c | 计算器/编辑器/浏览器 |

## 第 8 周：高级特性（阶段 G）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 36 | G1 | 管道(Pipe) | fd.c, entry.S | `cmd1 | cmd2` |
| 37 | G2 | 信号(Signal) | signal.c | Ctrl-C 处理 |
| 38 | G3 | TCP 协议栈 | tcp.c | SYN 包发送 |
| 39 | G4 | HTTP 协议 | http.c | GET 请求 |
| 40 | G5 | Framebuffer 控制台 | fb.c | 图形终端输出 |

## 第 9 周：附加主题（阶段 H）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 41 | H1 | 多地址空间 | page.c, task.c, entry.S | per-process PML4+CR3 |
| 42 | H2 | 抢占式调度 | entry.S (timer), tss.c | 定时器驱动进程切换 |
| 43 | H3 | AHCI 磁盘驱动 | ahci.c, pci.c | SATA 磁盘识别 |
| 44 | H4 | ext2 写入 | ext2.c | 文件创建+写入 |
| 45 | H5 | BusyBox 准备 | entry.S, fd.c, ramfs.c | 6 新 syscall |

## 第 10 周：深度扩展（阶段 I）✅

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 46 | I1 | COW Fork | page.c, entry.S | #PF 触发 COW |
| 47 | I2 | ELF PIE | elf.c, exec.c | ET_DYN 支持 |
| 48 | I3 | TCP 握手 | tcp.c | 三次握手 |
| 49 | I4 | HTTP 服务器 | http.c | GET→HTML |
| 50 | I5 | ext2 间接块 | ext2.c | >12KB 文件 |

## 第 11 周：新领域（阶段 J）

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 51 | J1 | UDP+DHCP | udp.c, dhcp.c (新) | 自动获取 IP |
| 52 | J2 | /proc 文件系统 | procfs.c (新) | /proc/meminfo |
| 53 | J3 | SMP 多核 | smp.c, apic.c (新) | AP 启动 |
| 54 | J4 | 自旋锁调度 | spinlock.c (新) | 多核并行 |
| 55 | J5 | RCU 同步 | rcu.c (新) | 宽限期等待 |

## 第 12 周：生态 + 收尾（阶段 K）

| Day | 课节 | 内容 | 文件 | 验收 |
|-----|------|------|------|------|
| 56 | K1 | USB 驱动 | usb.c (新) | HID 键盘 |
| 57 | K2 | 用户态 C | crt0.S, syscalls.c (新) | printf 运行 |
| 58 | K3 | 微内核 IPC | ipc.c (新) | send/recv |
| 59 | K4 | 内核调试器 | kdb.c (新) | reg/mem/bp |
| 60 | K5 | 集成测试 | 全部文件 | 全功能通过 |

## 卡点记录

### 2026-06-24: CR3 硬编码错误
- **现象**: setup_pagetables() 后 ret 崩溃
- **根因**: `$0x102000` 硬编码, pml4 漂移到 0x103000
- **修复**: `lea pml4(%rip), %rdx; movq %rdx, %cr3`

### 2026-06-24: TSS.RSP0 偏移量
- **现象**: ring3→ring0 过渡失败
- **根因**: RSP0 在 TSS offset 4, 代码错写 offset 8
- **修复**: `*(unsigned long *)(tss + 4) = stack_top`

### 2026-06-24: 堆边界覆盖
- **现象**: buddy 分配器返回的页面覆盖 bio_pool 导致 bi_end_io 丢失
- **根因**: heap_start 在 head.S BSS 末尾定义, 其他 .c 的 BSS 在后
- **修复**: linker.ld 添加 `_kernel_end = .;`
