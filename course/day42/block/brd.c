/*
 * block/brd.c — RAM 块设备驱动
 *
 * 对照: reference/linux-7.1/drivers/block/brd.c
 *
 * 最简 RAM 盘: 静态页数组做 backing store,
 * bio-based 直通(无请求队列, 无 blk-mq)。
 *
 * 数据结构:
 *   brd->pages[i] = 第 i 个扇区所在页的物理地址
 *   每页 4096 字节 = 8 扇区(512B/扇区)
 */

#include "kernel.h"
#include "blkdev.h"
#include "mm.h"

/* -------- 常量 -------- */
#define BRD_PAGE_SIZE    4096
#define SECTOR_SIZE      512
#define SECTORS_PER_PAGE (BRD_PAGE_SIZE / SECTOR_SIZE)  /* 8 */

/* brd 设备(单例) */
struct brd_device {
    struct gendisk *disk;
    unsigned long   nr_pages;      /* 总页数 */
    void          **pages;         /* 页指针数组 */
};

/* 静态页数组(固定 256 页 = 1MB RAM 盘) */
#define BRD_MAX_PAGES 256
static void *brd_page_array[BRD_MAX_PAGES];
static struct brd_device brd_dev;
static struct gendisk *brd_disk_ptr;  /* 全局引用, main.c 使用 */

/* ================================================================
 * brd_lookup_or_alloc_page — 查找或分配扇区对应的页
 * ================================================================ */
static void *brd_get_page(struct brd_device *brd, unsigned long sector)
{
    unsigned long page_idx = sector / SECTORS_PER_PAGE;

    if (page_idx >= brd->nr_pages)
        return 0;

    if (!brd->pages[page_idx]) {
        /* 首次写入: 分配整页(4096 字节, order=0) */
        void *p = alloc_pages(0);
        if (!p) return 0;
        brd->pages[page_idx] = p;
    }
    return brd->pages[page_idx];
}

/* ================================================================
 * brd_submit_bio — 处理一个 bio
 *
 * 对照: brd.c:brd_submit_bio() + brd_rw_bvec()
 *
 * 简化: 只处理单段 bio_vec, 无 discard
 * ================================================================ */
static void brd_submit_bio(struct bio *bio)
{
    struct brd_device *brd = bio->bi_bdev->bd_disk->private_data;
    struct bio_vec *bv = &bio->bi_io_vec;
    unsigned long sector = bio->bi_sector;
    unsigned int len = bv->bv_len;
    unsigned int offset = bv->bv_offset;
    void *data = bv->bv_page;

    if (len == 0) {
        bio_endio(bio);
        return;
    }

    /* 逐页处理 */
    while (len > 0) {
        unsigned int  page_off = (sector % SECTORS_PER_PAGE) * SECTOR_SIZE;
        unsigned int  chunk = SECTOR_SIZE * SECTORS_PER_PAGE - page_off;
        if (chunk > len) chunk = len;

        void *page = brd_get_page(brd, sector);
        if (!page && bio->bi_opf == REQ_OP_READ) {
            for (unsigned int i = 0; i < chunk; i++)
                ((char *)data)[offset + i] = 0;
        } else if (page) {
            if (bio->bi_opf == REQ_OP_WRITE) {
                for (unsigned int i = 0; i < chunk; i++)
                    ((char *)page)[page_off + i] = ((char *)data)[offset + i];
            } else {
                for (unsigned int i = 0; i < chunk; i++)
                    ((char *)data)[offset + i] = ((char *)page)[page_off + i];
            }
        } else {
            bio->bi_status = -1;
            break;
        }

        sector += chunk / SECTOR_SIZE;
        offset += chunk;
        len    -= chunk;
    }

    bio_endio(bio);
}

/* ================================================================
 * brd_fops — 块设备操作表
 *
 * 对照: brd.c 的 brd_fops, 只设 .submit_bio
 * ================================================================ */
static struct block_device_operations brd_fops = {
    .submit_bio = brd_submit_bio,
};

/* ================================================================
 * brd_init — 创建 RAM 块设备
 *
 * 对照: brd.c:brd_alloc()
 *
 * 创建一个 1MB RAM 盘(256 页 × 4096 字节)
 * ================================================================ */
void brd_init(void)
{
    struct brd_device *brd = &brd_dev;

    brd->nr_pages = BRD_MAX_PAGES;
    brd->pages    = brd_page_array;

    /* 分配 gendisk */
    struct gendisk *disk = blk_alloc_disk();
    if (!disk) {
        serial_puts("brd: no disk slot\r\n");
        return;
    }

    disk->major = 1;  /* RAM disk major */
    disk->fops  = &brd_fops;
    disk->private_data = brd;
    disk->capacity = brd->nr_pages * SECTORS_PER_PAGE;

    /* 名称 */
    disk->disk_name[0] = 'b';
    disk->disk_name[1] = 'r';
    disk->disk_name[2] = 'd';
    disk->disk_name[3] = '0';
    disk->disk_name[4] = '\0';

    brd->disk = disk;
    brd_disk_ptr = disk;  /* 导出给 main.c 直接使用 */
    add_disk(disk);

    serial_puts("brd: 1MB RAM disk ready (");
    print_hex64(disk->capacity);
    serial_puts(" sectors)\r\n");
}

/* 获取 brd 设备指针(供外部使用) */
struct gendisk *brd_get_disk(void)
{
    return brd_disk_ptr;
}
