/*
 * block/bio.c — bio 分配/提交/完成
 *
 * 对照: reference/linux-7.1/block/bio.c
 *       reference/linux-7.1/block/blk-core.c (submit_bio)
 *
 * bio-based 直通路径:
 *   submit_bio(bio) → bio->bi_bdev->bd_disk->fops->submit_bio(bio)
 *
 * 简化: 无请求队列, 无 blk-mq, 单段 bio_vec
 */

#include "kernel.h"
#include "blkdev.h"

/* 静态 bio 池(简化: 无动态分配) */
#define BIO_POOL_SIZE 8
static struct bio bio_pool[BIO_POOL_SIZE];
static int bio_pool_used[BIO_POOL_SIZE];

/* -------- 全局块设备注册表 -------- */
struct gendisk *blkdev_table[MAX_BLKDEV];

/* ================================================================
 * bio_alloc — 分配一个 bio
 * ================================================================ */
struct bio *bio_alloc(struct block_device *bdev, unsigned int opf)
{
    for (int i = 0; i < BIO_POOL_SIZE; i++) {
        if (!bio_pool_used[i]) {
            bio_pool_used[i] = 1;
            struct bio *bio = &bio_pool[i];
            bio->bi_bdev    = bdev;
            bio->bi_opf     = opf;
            bio->bi_status  = 0;
            bio->bi_sector  = 0;
            bio->bi_size    = 0;
            bio->bi_end_io  = 0;
            bio->bi_private = 0;
            bio->bi_io_vec.bv_page   = 0;
            bio->bi_io_vec.bv_len    = 0;
            bio->bi_io_vec.bv_offset = 0;
            return bio;
        }
    }
    return 0;  /* bio 池耗尽 */
}

/* ================================================================
 * bio_add_page — 添加数据段到 bio
 * ================================================================ */
void bio_add_page(struct bio *bio, void *page, unsigned int len, unsigned int offset)
{
    bio->bi_io_vec.bv_page   = page;
    bio->bi_io_vec.bv_len    = len;
    bio->bi_io_vec.bv_offset = offset;
    bio->bi_size = len;
}

/* ================================================================
 * submit_bio — 提交 I/O 请求
 *
 * 对照: submit_bio() → submit_bio_noacct() → __submit_bio()
 *       最终: disk->fops->submit_bio(bio)
 * ================================================================ */
void submit_bio(struct bio *bio)
{
    struct gendisk *disk = bio->bi_bdev->bd_disk;
    if (disk && disk->fops && disk->fops->submit_bio) {
        disk->fops->submit_bio(bio);
        return;
    }
    /* 无 submit_bio: 标记失败并完成 */
    bio->bi_status = -1;
    bio_endio(bio);
}

/* ================================================================
 * bio_endio — bio 完成(驱动调用)
 * 调用 bi_end_io 回调, 释放 bio
 * ================================================================ */
void bio_endio(struct bio *bio)
{
    if (bio->bi_end_io)
        bio->bi_end_io(bio);

    /* 释放 bio */
    for (int i = 0; i < BIO_POOL_SIZE; i++) {
        if (&bio_pool[i] == bio) {
            bio_pool_used[i] = 0;
            break;
        }
    }
}

/* ================================================================
 * blk_alloc_disk — 分配 gendisk(简化: 返回静态分配的)
 * ================================================================ */
/* 静态 gendisk 存储(全局, 不在函数内) */
static struct gendisk bio_disk_table[MAX_BLKDEV];

struct gendisk *blk_alloc_disk(void)
{
    for (int i = 0; i < MAX_BLKDEV; i++) {
        if (!bio_disk_table[i].fops) {
            return &bio_disk_table[i];
        }
    }
    return 0;
}

/* ================================================================
 * add_disk — 注册块设备
 * ================================================================ */
int add_disk(struct gendisk *disk)
{
    for (int i = 0; i < MAX_BLKDEV; i++) {
        if (!blkdev_table[i]) {
            blkdev_table[i] = disk;
            serial_puts("blk: registered ");
            serial_puts(disk->disk_name);
            serial_puts("\r\n");
            return 0;
        }
    }
    return -1;
}

/* ================================================================
 * blkdev_read / blkdev_write — 同步读写封装
 *
 * 用于文件系统层简单调用, 内部设 completion 回调同步等待。
 * ================================================================ */
static volatile int sync_done;

static void sync_end_io(struct bio *bio)
{
    (void)bio;
    sync_done = 1;
}

int blkdev_read(struct gendisk *disk, unsigned long sector,
                void *buf, unsigned int nr_sectors)
{
    static struct block_device bdev_r;
    bdev_r.bd_disk = disk;
    struct bio *bio = bio_alloc(&bdev_r, REQ_OP_READ);
    if (!bio) return -1;

    bio->bi_sector = sector;
    bio->bi_end_io = sync_end_io;
    bio_add_page(bio, buf, nr_sectors * 512, 0);

    sync_done = 0;
    submit_bio(bio);
    while (!sync_done)
        __asm__ volatile ("pause");  /* 自旋等待 */
    return 0;
}

int blkdev_write(struct gendisk *disk, unsigned long sector,
                 void *buf, unsigned int nr_sectors)
{
    static struct block_device bdev;
    bdev.bd_disk = disk;
    struct bio *bio = bio_alloc(&bdev, REQ_OP_WRITE);
    if (!bio) return -1;

    bio->bi_sector = sector;
    bio->bi_end_io = sync_end_io;
    bio_add_page(bio, buf, nr_sectors * 512, 0);

    sync_done = 0;
    submit_bio(bio);
    while (!sync_done)
        __asm__ volatile ("pause");
    return 0;
}
