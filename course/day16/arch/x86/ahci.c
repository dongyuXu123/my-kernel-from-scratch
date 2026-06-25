/*
 * arch/x86/ahci.c — AHCI SATA 磁盘驱动
 *
 * 对照: reference/linux-7.1/drivers/ata/ahci.c
 *       reference/linux-7.1/drivers/ata/libahci.c
 *
 * 实现: PCI 探测 → HBA 初始化 → IDENTIFY → READ/WRITE DMA
 * AHCI 规范: Intel AHCI 1.3.1
 *
 * 寄存器偏移 (ABAR MMIO):
 *   HBA_CAP     = 0x00   (Host Capability)
 *   HBA_GHC     = 0x04   (Global Host Control)
 *   HBA_IS      = 0x08   (Interrupt Status)
 *   HBA_PI      = 0x0C   (Ports Implemented)
 *   HBA_VS      = 0x10   (Version)
 *   Port 0 base = 0x100  (每端口 0x80 字节)
 *     PXCLB     = 0x00   (Command List Base)
 *     PXCLBU    = 0x04   (Command List Base Upper)
 *     PXFB      = 0x08   (FIS Base)
 *     PXFBU     = 0x0C   (FIS Base Upper)
 *     PXIS      = 0x10   (Interrupt Status)
 *     PXIE      = 0x14   (Interrupt Enable)
 *     PXCMD     = 0x18   (Command and Status)
 *     PXSSTS    = 0x28   (SATA Status)
 *     PXSERR    = 0x30   (SATA Error)
 *     PXSACT    = 0x38   (SATA Active)
 *     PXCI      = 0x3C   (Command Issue)
 */

#include "kernel.h"
#include "mm.h"
#include "device.h"
#include "blkdev.h"

/* PCI 辅助 (pci.c 内部定义, 此处 extern) */
struct pci_dev { int bus, dev, func; unsigned int vendor, device, class, bar0; };
extern unsigned int pci_read32(int bus, int dev, int func, int off);
extern void pci_write32(int bus, int dev, int func, int off, unsigned int val);

/* ---- AHCI 寄存器偏移 ---- */
#define HBA_CAP     0x00
#define HBA_GHC     0x04
#define HBA_PI      0x0C
#define PORT_BASE   0x100
#define PORT_STRIDE 0x80

/* Port 寄存器 */
#define PXCLB   0x00
#define PXFB    0x08
#define PXIS    0x10
#define PXIE    0x14
#define PXCMD   0x18
#define PXTFD   0x20
#define PXSIG   0x24
#define PXSSTS  0x28
#define PXSERR  0x30
#define PXSACT  0x38
#define PXCI    0x3C

/* PXCMD 位 */
#define PXCMD_ST    (1<<0)   /* Start */
#define PXCMD_FRE   (1<<4)   /* FIS Receive Enable */
#define PXCMD_FR    (1<<14)  /* FIS Receive Running */
#define PXCMD_CR    (1<<15)  /* Command List Running */

/* ATA 命令 */
#define ATA_IDENTIFY    0xEC
#define ATA_READ_DMA    0xC8
#define ATA_WRITE_DMA   0xCA

/* FIS 类型 */
#define FIS_TYPE_REG_H2D 0x27

/* ---- AHCI 数据结构 ---- */
struct hba_port {
    volatile unsigned int clb;      /* Command List Base */
    volatile unsigned int clbu;
    volatile unsigned int fb;       /* FIS Base */
    volatile unsigned int fbu;
    volatile unsigned int is;
    volatile unsigned int ie;
    volatile unsigned int cmd;
    volatile unsigned int rsv0;
    volatile unsigned int tfd;
    volatile unsigned int sig;
    volatile unsigned int ssts;
    volatile unsigned int sctl;
    volatile unsigned int serr;
    volatile unsigned int sact;
    volatile unsigned int ci;
    volatile unsigned int sntf;
    volatile unsigned int fbs;
};

struct hba_mem {
    volatile unsigned int cap;
    volatile unsigned int ghc;
    volatile unsigned int is;
    volatile unsigned int pi;
    volatile unsigned int vs;
    volatile unsigned int ccc_ctl;
    volatile unsigned int ccc_ports;
    volatile unsigned int em_loc;
    volatile unsigned int em_ctl;
    volatile unsigned int cap2;
    volatile unsigned int bohc;
    volatile unsigned char rsv[0xA0-0x2C];
    struct hba_port ports[1];
};

/* Command Header (32 bytes) */
struct ahci_cmd_header {
    unsigned short flags;
    unsigned short prdtl;       /* PRDT entries */
    unsigned int   prdbc;       /* bytes transferred */
    unsigned int   ctba;        /* Command Table Base */
    unsigned int   ctbau;
    unsigned int   rsv[4];
};

/* Command Table */
struct ahci_cmd_table {
    unsigned char cfis[64];     /* Command FIS */
    unsigned char acmd[16];     /* ATAPI */
    unsigned char rsv[48];
    unsigned char prdt[0];      /* PRDT entries */
};

/* PRDT Entry (16 bytes) */
struct ahci_prdt {
    unsigned int dba;           /* Data Base Address */
    unsigned int dbau;
    unsigned int rsv;
    unsigned int dbc:22;        /* Byte count */
    unsigned int rsv2:9;
    unsigned int i:1;           /* Interrupt on Complete */
};

/* FIS Register H2D */
struct fis_reg_h2d {
    unsigned char type;         /* 0x27 */
    unsigned char pmport:4;
    unsigned char rsv0:3;
    unsigned char c:1;          /* Command */
    unsigned char command;
    unsigned char featurel;
    unsigned char lba0, lba1, lba2;
    unsigned char device;
    unsigned char lba3, lba4, lba5;
    unsigned char featureh;
    unsigned char countl, counth;
    unsigned char icc;
    unsigned char control;
    unsigned char rsv1[4];
};

/* ---- 全局变量 ---- */
static volatile struct hba_mem *ahci_abar;
static struct gendisk *ahci_disk;
static int ahci_port_impl;

/* 物理内存页 (用于 DMA 结构) */
static void *cmd_list_page;     /* Command List */
static void *cmd_table_page;    /* Command Table + PRDT */
static void *fis_page;          /* Received FIS */

/* ---- 辅助函数 ---- */
static unsigned int ahci_read(volatile unsigned int *addr) {
    return *addr;
}

static void ahci_write(volatile unsigned int *addr, unsigned int val) {
    *addr = val;
}

/* ================================================================
 * ahci_port_init — 初始化 AHCI 端口
 * ================================================================ */
static int ahci_port_init(int port_idx)
{
    volatile struct hba_port *port = &ahci_abar->ports[port_idx];

    /* 停止端口 */
    ahci_write(&port->cmd, ahci_read(&port->cmd) & ~PXCMD_ST);
    ahci_write(&port->cmd, ahci_read(&port->cmd) & ~PXCMD_FRE);

    /* 等待 CR 和 FR 清零 */
    int timeout = 1000000;
    while ((ahci_read(&port->cmd) & (PXCMD_CR|PXCMD_FR)) && --timeout) {
        __asm__ volatile ("pause");
    }
    if (!timeout) {
        serial_puts("AHCI: port stop timeout\r\n");
        return -1;
    }

    /* 设置 Command List 物理地址 */
    unsigned long cl_phys = (unsigned long)cmd_list_page;
    ahci_write(&port->clb, (unsigned int)cl_phys);
    ahci_write(&port->clbu, 0);

    /* 设置 FIS 物理地址 */
    unsigned long fis_phys = (unsigned long)fis_page;
    ahci_write(&port->fb, (unsigned int)fis_phys);
    ahci_write(&port->fbu, 0);

    /* 清除错误 */
    ahci_write(&port->serr, 0xFFFFFFFF);
    ahci_write(&port->is, 0xFFFFFFFF);

    /* 启动端口 */
    ahci_write(&port->cmd, PXCMD_FRE);
    timeout = 1000000;
    while (!(ahci_read(&port->cmd) & PXCMD_FR) && --timeout) {
        __asm__ volatile ("pause");
    }
    ahci_write(&port->cmd, ahci_read(&port->cmd) | PXCMD_ST);
    timeout = 1000000;
    while (!(ahci_read(&port->cmd) & PXCMD_CR) && --timeout) {
        __asm__ volatile ("pause");
    }

    serial_puts("AHCI: port ");
    print_hex64(port_idx);
    serial_puts(" initialized, SATA status=");
    print_hex64(ahci_read(&port->ssts));
    serial_puts("\r\n");

    return 0;
}

/* ================================================================
 * ahci_issue_cmd — 发送 ATA 命令 (FIS + PRDT)
 * ================================================================ */
static int ahci_issue_cmd(int port_idx, int cmd, int is_write,
                          unsigned int lba, void *buf, int sectors)
{
    volatile struct hba_port *port = &ahci_abar->ports[port_idx];
    volatile struct ahci_cmd_header *cmd_hdr =
        (volatile struct ahci_cmd_header *)cmd_list_page;
    volatile struct ahci_cmd_table *cmd_tbl =
        (volatile struct ahci_cmd_table *)cmd_table_page;

    /* 清零 */
    cmd_hdr->flags = 5;           /* 5 PRDT entries? No, FIS length in DW */
    cmd_hdr->prdtl = 1;           /* 1 PRDT entry */
    cmd_hdr->prdbc = 0;
    cmd_hdr->ctba = (unsigned int)(unsigned long)cmd_table_page;
    cmd_hdr->ctbau = 0;

    /* 构建 FIS */
    for (int i = 0; i < 64; i++) ((unsigned char *)cmd_table_page)[i] = 0;
    volatile struct fis_reg_h2d *fis =
        (volatile struct fis_reg_h2d *)cmd_table_page;
    fis->type = FIS_TYPE_REG_H2D;
    fis->c = 1;                    /* Command bit */
    fis->command = cmd;
    fis->device = 0xE0;           /* LBA mode */
    fis->lba0 = lba & 0xFF;
    fis->lba1 = (lba >> 8) & 0xFF;
    fis->lba2 = (lba >> 16) & 0xFF;
    fis->lba3 = (lba >> 24) & 0xFF;
    fis->countl = sectors & 0xFF;
    fis->counth = (sectors >> 8) & 0xFF;

    /* 构建 PRDT */
    volatile struct ahci_prdt *prdt =
        (volatile struct ahci_prdt *)((unsigned char *)cmd_table_page + 128);
    prdt->dba = (unsigned int)(unsigned long)buf;
    prdt->dbau = 0;
    prdt->dbc = sectors * 512 - 1;  /* byte count - 1 */
    prdt->i = 1;

    /* 清除中断状态 */
    ahci_write(&port->is, 0xFFFFFFFF);

    /* 发送命令 */
    ahci_write(&port->ci, 1);

    /* 等待完成 */
    int timeout = 10000000;
    while ((ahci_read(&port->ci) & 1) && --timeout) {
        __asm__ volatile ("pause");
    }
    if (!timeout) {
        serial_puts("AHCI: command timeout\r\n");
        return -1;
    }

    return 0;
}

/* ================================================================
 * ahci_submit_bio — 处理块设备 I/O 请求
 *
 * 对照: reference/linux-7.1/drivers/ata/libahci.c (ahci_qc_issue)
 * ================================================================ */
static void ahci_submit_bio(struct bio *bio)
{
    int port = 0;  /* 只用 port 0 */
    unsigned long sector = bio->bi_sector;
    void *buf = (void *)bio->bi_io_vec.bv_page;
    int nr_sec = bio->bi_io_vec.bv_len / 512;

    int cmd = (bio->bi_opf == REQ_OP_READ) ? ATA_READ_DMA : ATA_WRITE_DMA;

    int ret = ahci_issue_cmd(port, cmd, (bio->bi_opf != REQ_OP_READ),
                              sector, buf, nr_sec);
    if (ret == 0)
        bio_endio(bio);
    else
        bio->bi_status = 1;  /* 标记失败 */
}

static struct block_device_operations ahci_fops = {
    .submit_bio = ahci_submit_bio,
};

/* ================================================================
 * ahci_probe — PCI probe 入口
 *
 * 对照: reference/linux-7.1/drivers/ata/ahci.c (ahci_init_one)
 * ================================================================ */
static int ahci_probe(struct device *dev)
{
    struct pci_dev *pci = (struct pci_dev *)dev->priv;
    serial_puts("AHCI: probing vendor=");
    print_hex64(pci->vendor);
    serial_puts(" device=");
    print_hex64(pci->device);
    serial_puts(" class=");
    print_hex64(pci->class);
    serial_puts("\r\n");

    /* 映射 ABAR (BAR5) */
    unsigned int bar5 = pci_read32(pci->bus, pci->dev, pci->func, 0x24);
    unsigned long abar_phys = bar5 & 0xFFFFFFF0;

    ahci_abar = (volatile struct hba_mem *)abar_phys;
    extern void map_mmio(unsigned long);
    map_mmio(abar_phys);

    /* 使能 Bus Mastering + MMIO */
    unsigned int cmd_reg = pci_read32(pci->bus, pci->dev, pci->func, 0x04);
    pci_write32(pci->bus, pci->dev, pci->func, 0x04,
                 cmd_reg | (1<<2) | (1<<1));

    /* 分配 DMA 页面 */
    extern void *alloc_page(void);
    cmd_list_page = alloc_page();
    cmd_table_page = alloc_page();
    fis_page = alloc_page();
    if (!cmd_list_page || !cmd_table_page || !fis_page) {
        serial_puts("AHCI: DMA page alloc failed\r\n");
        return -1;
    }

    /* HBA 复位 */
    ahci_write(&ahci_abar->ghc, 1);    /* GHC.AE = 0? No, 1 = HBA Reset */
    int timeout = 1000000;
    while ((ahci_read(&ahci_abar->ghc) & 1) && --timeout) {
        __asm__ volatile ("pause");
    }
    ahci_write(&ahci_abar->ghc, (1<<31));  /* GHC.AE = 1 (AHCI Enable) */

    ahci_port_impl = ahci_read(&ahci_abar->pi);

    /* 初始化第一个可用端口 */
    for (int i = 0; i < 32; i++) {
        if (ahci_port_impl & (1 << i)) {
            if (ahci_port_init(i) == 0) {
                ahci_disk = blk_alloc_disk();
                if (!ahci_disk) return -1;

                ahci_disk->major = 3;  /* AHCI major */
                ahci_disk->fops = &ahci_fops;
                ahci_disk->capacity = 204800;  /* ~100MB (简化) */
                ahci_disk->disk_name[0] = 's';
                ahci_disk->disk_name[1] = 'd';
                ahci_disk->disk_name[2] = 'a';
                ahci_disk->disk_name[3] = 0;
                add_disk(ahci_disk);

                serial_puts("AHCI: disk sda registered\r\n");
                return 0;
            }
        }
    }

    serial_puts("AHCI: no usable port found\r\n");
    return -1;
}

static struct driver ahci_driver = {
    .name = "ahci",
    .probe = ahci_probe,
    .pci_vendor = 0,        /* 使用 class 匹配 */
    .pci_device = 0,
    .pci_class  = 0x0106,  /* mass storage (0x01) + SATA (0x06) */
};

/* ================================================================
 * ahci_init — 模块入口 (在 pci_scan 后调用)
 * ================================================================ */
void ahci_init(void)
{
    extern struct bus_type *pci_bus_ptr;
    ahci_driver.bus = pci_bus_ptr;

    driver_register(&ahci_driver);
    serial_puts("AHCI: driver registered (class=0x0106)\r\n");
}
