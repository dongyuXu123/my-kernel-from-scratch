# H3: AHCI SATA 驱动 (Day 43)

## 对照源码
- `reference/linux-7.1/drivers/ata/ahci.c` (ahci_init_one)

## 学习目标
1. PCI class 扫描: class=0x01 (mass storage), subclass=0x06 (SATA)
2. AHCI HBA 寄存器: GHC (AE=1), PI (端口位图), Port 寄存器 (PXCLB/PXFB/PXCMD/PXCI)
3. FIS 构建: Register H2D (type=0x27) → IDENTIFY/READ DMA/WRITE DMA
4. PRDT: 描述 DMA 缓冲区 (物理地址 + 字节数)

## 代码导读
- `arch/x86/ahci.c`: ahci_probe → ahci_port_init → ahci_issue_cmd → ahci_submit_bio
- ABAR 映射: BAR5 → MMIO → map_mmio → HBA 寄存器访问

## 验证
```bash
cd course/day43 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
