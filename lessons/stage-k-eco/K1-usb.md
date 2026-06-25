# K1: USB 驱动 (Day 56)

## 对照源码
- `reference/linux-7.1/drivers/usb/host/ehci-hcd.c`

## 学习目标
1. USB 主机控制器: UHCI (USB 1.1) / EHCI (USB 2.0) / XHCI (USB 3.0)
2. 设备枚举: 获取描述符 → 设置地址 → 获取配置 → 设置配置
3. HID 键盘: 中断传输 → HID 报告解析 → 按键事件

## 代码导读
- `arch/x86/usb.c`: usb_init, usb_keyboard_init, usb_poll

## 验证
```bash
cd course/day56 && make
qemu-system-x86_64 -kernel mykernel.elf -usb -device usb-kbd \
  -display none -serial stdio -no-reboot -no-shutdown
```
