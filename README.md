# 🐧 MyKernel — Linux 内核从零手写

[![Build](https://img.shields.io/badge/build-60/60%20days-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()
[![Lines](https://img.shields.io/badge/code-~10000%20lines-orange)]()
[![Reference](https://img.shields.io/badge/reference-Linux%20v7.1-red)]()

> **60 天，逐行手写一个 x86-64 操作系统内核**  
> 对照 Linux v7.1 真实源码，从引导扇区到 GUI 桌面

---

## ✨ 特性

### 内核能力

| 类别 | 实现 |
|------|------|
| **引导** | multiboot1 + PVH ELF note → 32→64 长模式 |
| **内存** | Bitmap → Buddy → Slab 三级分配器 + 4 级页表 + map_mmio |
| **进程** | task_struct + fork + COW + 协作/抢占调度 + per-process 页表 |
| **文件系统** | ramfs + ext2 (只读→读写→间接块→符号链接→删除) + VFS |
| **系统调用** | 20 个 syscall (read/write/open/close/fork/exec/pipe/signal/...) |
| **驱动** | 8250 UART + 8259 PIC + 8253 PIT + PCI 枚举 + e1000 + AHCI |
| **网络** | Ethernet + ARP + ICMP + TCP(三次握手) + HTTP 服务器 + UDP + DHCP |
| **GUI** | PS/2 鼠标 → 窗口管理器 → 控件 → 桌面环境 → 应用程序 |
| **并发** | 自旋锁 + RCU + SMP 多核启动 |
| **调试** | 内核调试器 KDB (reg/mem/bp/step) |
| **其他** | 模块加载器 + /proc 伪文件系统 + 微内核 IPC + USB 驱动骨架 |

### 教学特性

- **每天一课，渐进式构建** — 每节课只加一个核心概念
- **每课可编译、可运行、可调试** — QEMU 验证
- **对照 Linux v7.1 真实源码** — 每文件标注 `对照: reference/linux-7.1/...`
- **汇编最少化** — C 实现所有逻辑，汇编仅 CPU 指令包装

---

## 🚀 快速开始

```bash
# 1. 进入任意课程天
cd course/day03

# 2. 编译
make

# 3. QEMU 运行
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown

# GUI 天 (Day 31-35) 需要:
qemu-system-x86_64 -kernel mykernel.elf -vga std -device i8042 -display none -serial stdio -no-reboot -no-shutdown
```

---

## 📚 课程路线图 (60 天)

| 周 | 天数 | 阶段 | 核心内容 |
|----|------|------|---------|
| 1 | 01-05 | 🔌 引导 | QEMU → 长模式 → 串口 → GDT/IDT → 异常 |
| 2 | 06-10 | 🧠 内存 | Bitmap → Buddy → Slab → 4级页表 → map_mmio |
| 3 | 11-15 | 🏃 进程 | task_struct → 上下文切换 → ring3 → syscall → fork |
| 4 | 16-20 | 📁 文件系统 | ramfs → ELF 加载器 → exec → Shell |
| 5 | 21-25 | 🔧 驱动 | 块设备 → ext2 只读 → 模块 → PCI/e1000 → 定时器 |
| 6 | 26-30 | 🌐 网络基础 | 设备模型 → 抢占调度 → ARP/ICMP → VFS → 集成 |
| 7 | 31-35 | 🖥️ GUI | 鼠标 → 窗口管理器 → 控件 → 桌面 → 应用 |
| 8 | 36-40 | 📡 进阶 | Pipe → Signal → TCP → HTTP → Framebuffer |
| 9 | 41-45 | 🔬 附加 | 多地址空间 → 抢占 → AHCI → ext2写 → syscall扩展 |
| 10 | 46-50 | 💎 深度 | COW fork → ELF PIE → TCP握手 → HTTPsrv → ext2间接块 |
| 11 | 51-55 | 🆕 新领域 | UDP+DHCP → /proc → SMP → spinlock → RCU |
| 12 | 56-60 | 🌍 生态 | USB → newlib → IPC → KDB → 集成测试 |

---

## 📖 项目结构

```
kernel-from-scratch/
├── mykernel/              # 完整内核源码 (60天全部特性)
│   ├── arch/x86/          # 架构相关 (boot, head, entry, drivers)
│   ├── kernel/            # 内核核心 (main, sched, task, fd, module, ...)
│   ├── mm/                # 内存管理 (pmm, buddy, slab, page)
│   ├── fs/                # 文件系统 (ramfs, ext2, elf, exec, vfs, procfs)
│   ├── net/               # 网络 (net, tcp, http, udp)
│   ├── block/             # 块设备 (bio, brd)
│   └── include/           # 头文件
├── course/                # 60 天课程 (每天独立目录)
│   ├── day01/             # 最小 32 位死循环
│   ├── day02/             # 长模式切换
│   ├── ...
│   └── day60/             # 最终集成
├── lessons/               # 详细课程文档
│   ├── stage-a-boot/      # 引导阶段
│   ├── stage-b-memory/    # 内存阶段
│   ├── stage-c-process/   # 进程阶段
│   ├── stage-d-fs/        # 文件系统阶段
│   ├── stage-e-net/       # 驱动+网络阶段
│   ├── stage-f-gui/       # GUI 阶段
│   ├── stage-g-adv/       # 进阶阶段
│   ├── stage-h-ext/       # 附加主题
│   ├── stage-i-deep/      # 深度扩展
│   ├── stage-j-new/       # 新领域
│   └── stage-k-eco/       # 生态收尾
├── reference/             # Linux v7.1 参考源码 (git clone)
├── ROADMAP.md             # 60 天路线图
├── progress.md            # 进度追踪
└── README.md              # 本文件
```

---

## 🎯 学习原则

1. **先读源码，再写代码** — 每节先读 `reference/linux-7.1` 对应文件
2. **每节可运行验证** — QEMU 即为验收标准
3. **汇编只做必须** — C 实现所有逻辑，汇编仅用于 CPU 指令包装
4. **渐进式构建** — 每天只加一个概念，不跳步

---

## 📊 统计数据

- **60 天** 课程
- **~85 个** 源文件
- **~10,000 行** 代码
- **20 个** 系统调用
- **12 个** 设备驱动
- **参考**: Linux v7.1 (2025)

---

## 🔗 参考资源

- [Linux Kernel v7.1 Source](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git)
- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel x86-64 Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [AHCI Specification](https://www.intel.com/content/www/us/en/io/serial-ata/ahci-specification.html)
- [tinywm](https://github.com/mackstann/tinywm) — 极简窗口管理器参考

---

## 📄 License

MIT — 自由使用、修改、分发
