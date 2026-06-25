/*
 * include/blkdev.h — 块设备层 API
 *
 * 对照: reference/linux-7.1/include/linux/blkdev.h
 *       reference/linux-7.1/include/linux/blk_types.h
 *
 * 简化设计(bio-based, 跳过 blk-mq):
 *   bio → submit_bio() → gendisk->fops->submit_bio() → 驱动直接处理
 *
 * 对照真实内核 brd.c(RAM disk): 最简 bio-based 驱动
 */

#ifndef _BLKDEV_H
#define _BLKDEV_H

/* -------- I/O 操作类型 -------- */
#define REQ_OP_READ  0
#define REQ_OP_WRITE 1

/* -------- bio_vec: 一个内存段描述符 -------- */
struct bio_vec {
    void         *bv_page;     /* 数据页指针 */
    unsigned int  bv_len;      /* 本段字节数 */
    unsigned int  bv_offset;   /* 页内偏移 */
};

/* -------- bio: 一个 I/O 请求 --------
 * 对照: struct bio (include/linux/blk_types.h:210)
 * 简化: 单段 bio_vec, 无链式/拆分
 */
struct bio {
    struct block_device *bi_bdev;   /* 目标块设备 */
    unsigned int         bi_opf;    /* REQ_OP_READ / REQ_OP_WRITE */
    int                  bi_status; /* 0=成功, 非0=错误 */
    unsigned long        bi_sector; /* 起始扇区(512字节) */
    unsigned int         bi_size;   /* 总字节数 */
    struct bio_vec       bi_io_vec; /* 单个数据段(简化) */
    void               (*bi_end_io)(struct bio *); /* 完成回调 */
    void                *bi_private; /* 回调私有数据 */
};

/* -------- gendisk: 一个块设备 --------
 * 对照: struct gendisk (include/linux/blkdev.h:146)
 * 简化: 无分区, 无 queue, 只有 submit_bio 回调
 */
struct block_device_operations {
    void (*submit_bio)(struct bio *bio);
};

struct gendisk {
    int major;
    char disk_name[32];
    unsigned long capacity;               /* 总扇区数(512B/扇区) */
    struct block_device_operations *fops; /* 驱动操作 */
    void *private_data;                   /* 驱动私有数据 */
};

/* 内嵌的 block_device(简化: 就是 gendisk 本身) */
struct block_device {
    struct gendisk *bd_disk;
};

/* -------- 全局块设备注册表 -------- */
#define MAX_BLKDEV 4
extern struct gendisk *blkdev_table[MAX_BLKDEV];

/* -------- API -------- */
extern struct bio *bio_alloc(struct block_device *bdev, unsigned int opf);
extern void bio_add_page(struct bio *bio, void *page, unsigned int len, unsigned int offset);
extern void submit_bio(struct bio *bio);
extern void bio_endio(struct bio *bio);

extern struct gendisk *blk_alloc_disk(void);
extern int add_disk(struct gendisk *disk);

/* 简易读写封装 */
extern int blkdev_read(struct gendisk *disk, unsigned long sector,
                       void *buf, unsigned int nr_sectors);
extern int blkdev_write(struct gendisk *disk, unsigned long sector,
                        void *buf, unsigned int nr_sectors);

#endif /* _BLKDEV_H */
