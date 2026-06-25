# Day 21: 块设备层 — bio + RAM 盘

> 对照: reference/linux-7.1/block/bio.c, drivers/block/brd.c

## 1. 目标
- 实现 bio_alloc→bio_add_page→submit_bio→bio_endio 生命周期
- 实现 RAM 盘驱动(brd): 静态页数组 backing store
- **验收**: blkdev_write→blkdev_read 数据一致(PASS)

## 2. 关键实现
- `include/blkdev.h` — struct bio, gendisk, block_device_operations
- `block/bio.c` — 静态 bio 池(8个), submit_bio→disk->fops->submit_bio, sync_end_io 同步等待
- `block/brd.c` — brd_submit_bio: 逐扇区读写页数组, bio_endio 完成

## 3. bio-based 直通路径
```
submit_bio(bio) → disk->fops->submit_bio(bio) → 驱动直接处理
(跳过 blk-mq, 无请求队列)
```
