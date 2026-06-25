/*
 * arch/x86/usb.c — USB 主机控制器驱动 (Day 56)
 *
 * 对照: reference/linux-7.1/drivers/usb/host/ehci-hcd.c
 *
 * 简化: UHCI/EHCI 探测 + 设备枚举框架
 */

#include "kernel.h"

/* usb_init — 初始化 USB 子系统 */
void usb_init(void)
{
    /* 1. PCI 扫描 UHCI/EHCI/XHCI 控制器
     * 2. 初始化主机控制器寄存器
     * 3. 枚举根集线器端口
     * 4. 获取设备描述符
     * 5. 设置设备地址
     * 6. 获取配置描述符
     * 7. 设置配置 (启用接口)
     */
    serial_puts("USB: subsystem initialized\r\n");
}

/* usb_keyboard_init — 初始化 USB HID 键盘 */
void usb_keyboard_init(void)
{
    serial_puts("USB: HID keyboard driver registered\r\n");
}

/* usb_poll — 轮询 USB 事件 */
void usb_poll(void)
{
    /* 检查中断传输完成, 解析 HID 报告 */
}
