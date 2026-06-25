# Day 43: AHCI SATA 磁盘驱动

## 学习目标
- 理解 AHCI 规范：HBA 寄存器、Command List、FIS、PRDT
- PCI class 扫描 (class=0x01, subclass=0x06 = SATA)
- HBA 初始化：端口启动、Command/FIS 基址设置
- 构建 FIS 发送 ATA 命令 (IDENTIFY / READ DMA / WRITE DMA)
- 注册为 `struct gendisk` 块设备 (/dev/sda)

## 基础知识

### AHCI 寄存器 (ABAR MMIO)
```
HBA_CAP  (0x00): 主机能力
HBA_GHC  (0x04): 全局控制 (bit31 AE=1 启用)
HBA_PI   (0x0C): 已实现端口位图
Port 0   (0x100): 每端口 0x80 字节
  PXCLB  (0x00): Command List 基址 (物理地址)
  PXFB   (0x08): FIS 接收区基址
  PXCMD  (0x18): ST=启动, FRE=FIS接收使能
  PXCI   (0x3C): 命令提交 (写 1 触发)
```

### FIS (Register H2D, type=0x27)
```
Byte 0: type=0x27
Byte 1: C=1 (Command)
Byte 2: ATA 命令 (IDENTIFY=0xEC, READ DMA=0xC8, WRITE DMA=0xCA)
Byte 4-7: LBA[0..27]
Byte 8-10: LBA[28..47]
Byte 12-13: 扇区数
```

## 代码文件
- `arch/x86/ahci.c` — AHCI 驱动 (PCI probe + 端口初始化 + ATA 命令)
- `arch/x86/pci.c` — PCI class 扫描 + class 匹配
- `include/device.h` — driver 添加 `pci_class` 字段

## QEMU 验证
```bash
cd course/day43
make
timeout 4 qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
# 预期: AHCI 驱动注册，检测到 SATA 磁盘后注册为 /dev/sda
```
