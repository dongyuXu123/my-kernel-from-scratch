/*
 * include/device.h — 设备驱动模型
 *
 * 对照: reference/linux-7.1/include/linux/device.h
 *
 * 简化: bus → driver/device 链表, match→probe
 */

#ifndef _DEVICE_H
#define _DEVICE_H

struct device;
struct driver;

/* ---- bus ---- */
struct bus_type {
    const char *name;
    struct device *devices;       /* 设备链表 */
    struct driver *drivers;       /* 驱动链表 */
    int (*match)(struct device *dev, struct driver *drv);
};

/* ---- device ---- */
struct device {
    const char      *name;
    struct bus_type *bus;
    struct device   *next;        /* 链表 next */
    void            *priv;        /* 私有数据(如 PCI info) */
    struct driver   *drv;         /* 绑定的驱动 */
};

/* ---- driver ---- */
struct driver {
    const char      *name;
    struct bus_type *bus;
    struct driver   *next;
    int (*probe)(struct device *dev);
    /* PCI 匹配信息(可选) */
    unsigned int    pci_vendor;
    unsigned int    pci_device;
    unsigned int    pci_class;     /* class/subclass (vendor=0 时使用) */
};

/* ---- API ---- */
extern void bus_register(struct bus_type *bus);
extern void device_register(struct device *dev);
extern void driver_register(struct driver *drv);

/* 辅助: 遍历设备/驱动 */
extern void devices_show(void);

#endif
