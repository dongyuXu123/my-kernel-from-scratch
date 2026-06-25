/*
 * arch/x86/pci.c — PCI 总线枚举(设备模型版)
 *
 * 对照: reference/linux-7.1/arch/x86/pci/early.c (port I/O)
 *       reference/linux-7.1/drivers/pci/access.c (generic access)
 */
#include "kernel.h"
#include "device.h"

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

unsigned int pci_read32(int bus, int dev, int func, int off) {
    unsigned int a = 0x80000000 | ((unsigned)bus << 16) | ((dev & 0x1F) << 11) | ((func & 7) << 8) | (off & 0xFC);
    outl(PCI_ADDR, a); return inl(PCI_DATA);
}
unsigned short pci_read16(int bus, int dev, int func, int off) {
    unsigned int v = pci_read32(bus, dev, func, off & ~3);
    return (unsigned short)((v >> ((off & 2) * 8)) & 0xFFFF);
}
void pci_write32(int bus, int dev, int func, int off, unsigned int val) {
    unsigned int a = 0x80000000 | ((unsigned)bus << 16) | ((dev & 0x1F) << 11) | ((func & 7) << 8) | (off & 0xFC);
    outl(PCI_ADDR, a); outl(PCI_DATA, val);
}

struct pci_dev { int bus, dev, func; unsigned int vendor, device, class, bar0; };
static int pci_match(struct device *d, struct driver *drv) {
    struct pci_dev *p = d->priv;
    /* class 匹配: driver.pci_vendor==0 表示用 class/subclass/progif 匹配 */
    if (drv->pci_vendor == 0 && drv->pci_device == 0 && drv->pci_class != 0) {
        return (p->class >> 16) == (drv->pci_class & 0xFFFF);
    }
    return (p->vendor == drv->pci_vendor && p->device == drv->pci_device);
}
struct bus_type pci_bus = { .name = "pci", .match = pci_match };
struct bus_type *pci_bus_ptr;
static struct device pci_devs[32]; static struct pci_dev pci_priv[32]; static int nr;

void pci_scan(void) {
    bus_register(&pci_bus); pci_bus_ptr = &pci_bus;
    for (int dev = 0; dev < 32; dev++) {
        unsigned int v = pci_read16(0, dev, 0, 0);
        if (v == 0xFFFF) continue;
        unsigned int d = pci_read16(0, dev, 0, 2);
        struct device *dd = &pci_devs[nr];
        struct pci_dev *pp = &pci_priv[nr];
        pp->vendor = v; pp->device = d;
        pp->class = pci_read32(0, dev, 0, 0x08);  /* class code */
        pp->bar0 = pci_read32(0, dev, 0, 0x10);
        /* 开启 Bus Mastering */
        unsigned int cmd = pci_read32(0, dev, 0, 0x04);
        pci_write32(0, dev, 0, 0x04, cmd | (1 << 2));
        dd->name = "pci"; dd->bus = &pci_bus; dd->priv = pp; dd->drv = 0;
        serial_puts("PCI: "); print_hex64(dev); serial_puts(" v=");
        print_hex64(v); serial_puts(" d="); print_hex64(d); serial_puts("\r\n");
        device_register(dd); nr++;
    }
}
unsigned int pci_get_bar0(struct device *d) { return d ? ((struct pci_dev *)d->priv)->bar0 : 0; }
