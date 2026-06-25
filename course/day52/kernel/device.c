/*
 * kernel/device.c — 设备驱动模型
 *
 * 对照: reference/linux-7.1/drivers/base/core.c, bus.c, driver.c
 */

#include "kernel.h"
#include "device.h"

/* ================================================================
 * bus_register — 注册一个总线
 * ================================================================ */
void bus_register(struct bus_type *bus)
{
    serial_puts("bus: ");
    serial_puts(bus->name);
    serial_puts(" registered\r\n");
}

/* ================================================================
 * device_register — 注册设备, 尝试匹配驱动
 * ================================================================ */
void device_register(struct device *dev)
{
    /* 加入总线设备链表 */
    dev->next = dev->bus->devices;
    dev->bus->devices = dev;

    /* 尝试匹配驱动 */
    for (struct driver *drv = dev->bus->drivers; drv; drv = drv->next) {
        if (dev->bus->match && dev->bus->match(dev, drv)) {
            dev->drv = drv;
            serial_puts("device: ");
            serial_puts(dev->name);
            serial_puts(" -> driver ");
            serial_puts(drv->name);
            serial_puts("\r\n");
            if (drv->probe)
                drv->probe(dev);
            return;
        }
    }
    serial_puts("device: ");
    serial_puts(dev->name);
    serial_puts(" no driver\r\n");
}

/* ================================================================
 * driver_register — 注册驱动, 尝试匹配设备
 * ================================================================ */
void driver_register(struct driver *drv)
{
    /* 加入总线驱动链表 */
    drv->next = drv->bus->drivers;
    drv->bus->drivers = drv;

    /* 尝试匹配已有设备 */
    for (struct device *dev = drv->bus->devices; dev; dev = dev->next) {
        if (!dev->drv && drv->bus->match && drv->bus->match(dev, drv)) {
            dev->drv = drv;
            serial_puts("driver: ");
            serial_puts(drv->name);
            serial_puts(" -> device ");
            serial_puts(dev->name);
            serial_puts("\r\n");
            if (drv->probe)
                drv->probe(dev);
        }
    }
}

/* ================================================================
 * devices_show — 列出所有总线上的设备
 * ================================================================ */
void devices_show(void)
{
    /* 简化: 遍历所有设备 */
    serial_puts("Devices:\r\n");
}
