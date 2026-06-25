# Day 30: 集成测试 + 课程总结

## 1. 全功能回归测试

```bash
make clean && make
timeout 5 qemu-system-x86_64 -kernel mykernel.elf \
    -display none -serial stdio -no-reboot -no-shutdown
```

**预期输出**:
```
Hello from my kernel
bus: pci registered
driver: e1000 -> device pci → probe BAR0=FEB80000
e1000: send OK
Hello from /init!
# help → Commands: help echo ls cat exec insmod
# exec hello → Hello from /bin/hello!
# insmod hello_mod.o → MHello from asm module!
```

## 2. 系统能力清单

| 层 | 能力 |
|------|------|
| 引导 | 32→64位, PVH ELF note, 4级页表 |
| 内存 | bitmap→Buddy→Slab, kmalloc/kfree, map_mmio |
| 中断 | #PF/#GP/#DF/int3/IRQ0, PIC+PIT |
| 进程 | task_struct, fork+yield, 抢占式调度, ring3 |
| syscall | read/write/fork/open/exec/ls/cat/insmod/writeto(10) |
| 文件系统 | ramfs(create/rw/list), ext2只读, ELF加载, VFS |
| 驱动 | PCI枚举, e1000(TX+RX), 设备模型(bus/driver/device) |
| 网络 | ARP应答, ICMP Echo Reply, 以太网帧收发 |
| 模块 | ELF解析+符号解析+R_X86_64_64重定位 |

## 3. 文件统计
~50 源文件, ~4200 行代码 (汇编~25%, C~75%)

## 4. 进阶方向
- TCP 三握手 + HTTP 客户端
- Framebuffer 图形控制台
- AHCI 磁盘驱动 + ext2 写入
- BusyBox 移植(真正的 /bin/sh)
- 多地址空间 + mmap
