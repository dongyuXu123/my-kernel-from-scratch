/*
 * arch/x86/e1000.c — Intel e1000 PCI 驱动(设备模型版)
 *
 * 对照: reference/linux-7.1/drivers/net/ethernet/intel/e1000/e1000_main.c
 */
#include "kernel.h"
#include "mm.h"
#include "device.h"

static void *e1000_mmio;
static int e1000_found;

static inline unsigned int mr(unsigned int o)
    { return *(volatile unsigned int *)((char *)e1000_mmio + o); }
static inline void mw(unsigned int o, unsigned int v)
    { *(volatile unsigned int *)((char *)e1000_mmio + o) = v; }

struct tx_desc { unsigned long a; unsigned short l; unsigned char cso,cmd,sta,css; unsigned short s; };
struct rx_desc { unsigned long a; unsigned short l,c; unsigned char st,er; unsigned short s; };
#define ND 8
static struct tx_desc *tx; static char *txb[ND];
static struct rx_desc *rx; static char *rxb[ND];

extern void map_mmio(unsigned long);
extern unsigned int pci_get_bar0(struct device *d);
extern void *alloc_pages(int);

/* ================================================================
 * probe — 驱动匹配后调用
 * ================================================================ */
static int e1000_probe(struct device *dev)
{
    unsigned int bar0 = pci_get_bar0(dev);
    e1000_mmio = (void *)(unsigned long)(bar0 & 0xFFFFFFF0);
    e1000_found = 1;
    map_mmio((unsigned long)e1000_mmio);

    /* PCI Bus Mastering */
    /* (already set by pci_scan? No, need to set it here) */
    /* Read BAR info from priv */
    struct pci_dev { int bus, dev, func; unsigned int vendor, device, class, bar0; };
    struct pci_dev *p = dev->priv;

    serial_puts("e1000: probed BAR0=");
    print_hex64((unsigned long)e1000_mmio);
    serial_puts("\r\n");

    /* 复位 */
    mw(0x0000, mr(0x0000) | (1 << 26));
    for (volatile int t = 0; t < 1000000; t++)
        if (!(mr(0x0000) & (1 << 26))) break;
    serial_puts("e1000: reset ok\r\n");

    /* MAC — 使用 EEPROM 默认值 (不覆盖) */
    unsigned int ral = mr(0x5400), rah = mr(0x5404);
    serial_puts("e1000: MAC ");
    print_hex64(ral); serial_puts(" "); print_hex64(rah);
    serial_puts("\r\n");

    /* TX 环 */
    tx = alloc_pages(0);
    for (int i = 0; i < ND; i++) { txb[i] = alloc_pages(0); tx[i].a = 0; tx[i].cmd = 0; tx[i].sta = 0; }
    mw(0x3800, (unsigned long)tx); mw(0x3804, 0); mw(0x3808, ND * 16); mw(0x3810, 0); mw(0x3818, 0);
    mw(0x0400, (1 << 1) | (1 << 3));  /* TCTL: EN=1, PSP=1 */

    /* RX 环 */
    rx = alloc_pages(0);
    for (int i = 0; i < ND; i++) { rxb[i] = alloc_pages(0); rx[i].a = (unsigned long)rxb[i]; rx[i].st = 0; }
    mw(0x2800, (unsigned long)rx); mw(0x2804, 0); mw(0x2808, ND * 16); mw(0x2810, 0); mw(0x2818, ND - 1);
    /* RCTL: RXEN + SBP + UPE + MPE + BSEX + BSIZE=00 */
    mw(0x0100, (1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<15));

    /* 发送测试包 */
    char *pkt = txb[0];
    for (int i = 0; i < 6; i++) pkt[i] = 0xFF;
    pkt[6]=0x52; pkt[7]=0x54; pkt[8]=0x00; pkt[9]=0x12; pkt[10]=0x34; pkt[11]=0x56;
    pkt[12]=0x08; pkt[13]=0x00;
    pkt[14]='H'; pkt[15]='E'; pkt[16]='L'; pkt[17]='L'; pkt[18]='O';
    tx[0].a = (unsigned long)pkt; tx[0].l = 19; tx[0].cmd = 0x09; tx[0].sta = 0;
    mw(0x3818, 1);
    for (int t = 0; t < 100000; t++) if (tx[0].sta & 0x01) break;
    serial_puts("e1000: send "); serial_puts((tx[0].sta & 0x01) ? "OK\r\n" : "TIMEOUT\r\n");
    return 0;
}

int e1000_send_packet(void *data, int len)
{
    if (!e1000_found || !tx) return -1;
    for (int i = 0; i < len; i++) txb[0][i] = ((char *)data)[i];
    tx[0].a = (unsigned long)txb[0]; tx[0].l = len; tx[0].cmd = 0x09; tx[0].sta = 0;
    mw(0x3818, 1);
    for (int t = 0; t < 100000; t++) if (tx[0].sta & 0x01) return len;
    return -1;
}

void e1000_poll(void)
{
    if (!e1000_found || !rx) return;
    extern void net_poll(void *, int);
    static int tail, poll_cnt;
    unsigned int rdh = mr(0x2810);
    
    poll_cnt++;
    if (poll_cnt == 1) {
        serial_puts("e1000: RX poll started, rdh=");
        print_hex64(rdh);
        serial_puts(" tail=");
        print_hex64(tail);
        serial_puts("\r\n");
    }
    
    while (tail != (int)rdh) {
        struct rx_desc *d = &rx[tail];
        if (!(d->st & 0x01)) break;
        serial_puts("e1000: RX packet! len=");
        print_hex64(d->l);
        serial_puts("\r\n");
        net_poll(rxb[tail], (int)d->l);
        d->st = 0;
        tail = (tail + 1) % ND;
        mw(0x2818, (tail + ND - 1) % ND);
    }
}

/* 注册驱动 */
static struct driver e1000_drv = {
    .name = "e1000",
    .bus  = 0,  /* 在 pci_scan 后由 main 设置 */
    .probe = e1000_probe,
    .pci_vendor = 0x8086,
    .pci_device = 0x100E,
};

void e1000_init(void)
{
    /* 驱动注册由 main.c 在 pci_scan 之后调用 */
    extern struct bus_type *pci_bus_ptr;
    e1000_drv.bus = pci_bus_ptr;
    driver_register(&e1000_drv);
}
