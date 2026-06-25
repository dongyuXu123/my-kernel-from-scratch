# Day 26: 设备驱动模型

> 对照: reference/linux-7.1/drivers/base/

## 1. 目标
- bus_type/device/driver 抽象, 总线注册, 设备/驱动注册, match→probe
- PCI 总线 match(vendor/device), e1000 驱动自动匹配
- **验收**: `bus: pci`→`device: pci`→`driver: e1000 -> device pci`→probe 调用

## 2. 关键实现
- `include/device.h` — struct bus_type{match}, device{bus,priv}, driver{probe,pci_vendor/device}
- `kernel/device.c` — bus_register, device_register(尝试match), driver_register(尝试match)
- `arch/x86/pci.c` — pci_match: 比较 vendor/device ID; pci_scan 注册设备到 pci_bus
- `arch/x86/e1000.c` — e1000_drv: .pci_vendor=0x8086, .pci_device=0x100E, .probe=e1000_probe
