/*
 * fs/ext2_test.c — 在 RAM 盘上构建 ext2 镜像并验证
 *
 * 对照: mkfs.ext2 的逻辑简化版
 *
 * 构建一个小 ext2 文件系统(1 group, 1024B blocks):
 *   block 0:   boot(无)
 *   block 1:   superblock
 *   block 2:   group descriptor
 *   block 3:   block bitmap
 *   block 4:   inode bitmap
 *   block 5-8: inode table(32 inodes × 128B = 4096B)
 *   block 9+:  data blocks
 *
 * 文件:
 *   / (root, inode 2, dir)
 *   /hello.txt (inode 12, regular file, content="Hello ext2!")
 */

#include "kernel.h"
#include "blkdev.h"

/* ext2 结构(与 ext2.c 中的定义一致) */
struct ext2_super_block {
    unsigned int  s_inodes_count;
    unsigned int  s_blocks_count;
    unsigned int  s_r_blocks_count;
    unsigned int  s_free_blocks_count;
    unsigned int  s_free_inodes_count;
    unsigned int  s_first_data_block;
    unsigned int  s_log_block_size;
    unsigned int  s_log_frag_size;
    unsigned int  s_blocks_per_group;
    unsigned int  s_frags_per_group;
    unsigned int  s_inodes_per_group;
    unsigned int  s_mtime;
    unsigned int  s_wtime;
    unsigned short s_mnt_count;
    short         s_max_mnt_count;
    unsigned short s_magic;
    unsigned short s_state;
    unsigned short s_errors;
    short         s_minor_rev_level;
    unsigned int   s_lastcheck;
    unsigned int   s_checkinterval;
    unsigned int   s_creator_os;
    unsigned int   s_rev_level;
    unsigned short s_def_resuid;
    unsigned short s_def_resgid;
    unsigned int   s_first_ino;
    unsigned short s_inode_size;
    char pad[898];  /* 填充到 1024 字节 */
};

struct ext2_group_desc {
    unsigned int  bg_block_bitmap;
    unsigned int  bg_inode_bitmap;
    unsigned int  bg_inode_table;
    unsigned short bg_free_blocks_count;
    unsigned short bg_free_inodes_count;
    unsigned short bg_used_dirs_count;
    unsigned short bg_pad;
    unsigned int  bg_reserved[3];
};

struct ext2_inode {
    unsigned short i_mode;
    unsigned short i_uid;
    unsigned int   i_size;
    unsigned int   i_atime;
    unsigned int   i_ctime;
    unsigned int   i_mtime;
    unsigned int   i_dtime;
    unsigned short i_gid;
    unsigned short i_links_count;
    unsigned int   i_blocks;
    unsigned int   i_flags;
    unsigned int   i_osd1;
    unsigned int   i_block[15];
    unsigned int   i_generation;
    unsigned int   i_file_acl;
    unsigned int   i_dir_acl;
    unsigned int   i_faddr;
    char i_osd2[12];
};

/* 目录项 */
struct ext2_dir_entry {
    unsigned int   inode;
    unsigned short rec_len;
    unsigned char  name_len;
    unsigned char  file_type;
    char           name[];
};

#define EXT2_BLOCK_SIZE   1024
#define EXT2_INODE_SIZE   128
#define EXT2_SUPER_MAGIC  0xEF53
#define S_IFDIR  0x4000
#define S_IFREG  0x8000

/* 磁盘布局常量(1024B 块) */
#define SB_BLOCK      1    /* superblock */
#define GD_BLOCK      2    /* group descriptor */
#define BM_BLOCK      3    /* block bitmap */
#define IM_BLOCK      4    /* inode bitmap */
#define IT_START      5    /* inode table start */
#define IT_SIZE       4    /* 4 blocks = 32 inodes */
#define DATA_START    9    /* first data block */

#define ROOT_INO      2
#define HELLO_INO     12

/* 写 RAM 盘辅助 */
static void write_block(struct gendisk *disk, unsigned int blk, void *data)
{
    blkdev_write(disk, blk * 2, data, 2);  /* 1024B = 2 sectors */
}
static void clear_block(struct gendisk *disk, unsigned int blk)
{
    static char zero[1024];
    write_block(disk, blk, zero);
}

/* ================================================================
 * ext2_mkfs — 在 RAM 盘上创建 ext2 文件系统
 * ================================================================ */
void ext2_mkfs(struct gendisk *disk)
{
    char buf[1024];

    serial_puts("ext2_mkfs: building filesystem...\r\n");

    /* 清零关键区域 */
    clear_block(disk, 1);
    clear_block(disk, 2);
    clear_block(disk, 3);
    clear_block(disk, 4);
    clear_block(disk, 5);
    clear_block(disk, 6);

    /* -------- superblock -------- */
    struct ext2_super_block *sb = (struct ext2_super_block *)buf;
    for (int i = 0; i < 1024; i++) buf[i] = 0;

    sb->s_inodes_count      = 32;       /* 32 inodes */
    sb->s_blocks_count      = 32;       /* 32 blocks total */
    sb->s_free_blocks_count = 32 - 9;   /* data blocks are free */
    sb->s_free_inodes_count = 32 - 11;  /* first_ino=11, so 21 free */
    sb->s_first_data_block  = 1;        /* superblock at block 1 */
    sb->s_log_block_size    = 0;        /* 1024 << 0 = 1024 */
    sb->s_blocks_per_group  = 32;       /* all blocks in one group */
    sb->s_inodes_per_group  = 32;
    sb->s_magic             = EXT2_SUPER_MAGIC;
    sb->s_state             = 0;        /* clean */
    sb->s_rev_level         = 0;        /* good old rev */
    sb->s_first_ino         = 11;       /* first non-reserved */
    sb->s_inode_size        = 128;
    write_block(disk, SB_BLOCK, buf);

    /* -------- group descriptor -------- */
    for (int i = 0; i < 1024; i++) buf[i] = 0;
    struct ext2_group_desc *gd = (struct ext2_group_desc *)buf;
    gd->bg_block_bitmap    = BM_BLOCK;
    gd->bg_inode_bitmap    = IM_BLOCK;
    gd->bg_inode_table     = IT_START;
    gd->bg_free_blocks_count = 32 - 9;  /* data blocks */
    gd->bg_free_inodes_count = 32 - 11; /* non-reserved */
    gd->bg_used_dirs_count = 1;          /* root dir */
    write_block(disk, GD_BLOCK, buf);

    /* -------- block bitmap: mark blocks 1-8 used -------- */
    for (int i = 0; i < 1024; i++) buf[i] = 0xFF;  /* all used initially */
    for (int b = DATA_START; b < 32; b++) {
        int byte = b / 8;
        int bit  = b % 8;
        buf[byte] &= ~(1 << bit);
    }
    write_block(disk, BM_BLOCK, buf);

    /* -------- inode bitmap: mark 1-11 used, 12+ free -------- */
    for (int i = 0; i < 1024; i++) buf[i] = 0;  /* all free */
    /* mark inodes 1-11 as used */
    for (int ino = 1; ino <= 11; ino++) {
        int byte = ino / 8;
        int bit  = ino % 8;
        buf[byte] |= (1 << bit);
    }
    write_block(disk, IM_BLOCK, buf);

    /* -------- root inode(inode 2, directory) -------- */
    struct ext2_inode root;
    for (int i = 0; i < sizeof(root); i++) ((char *)&root)[i] = 0;
    root.i_mode  = S_IFDIR | 0755;
    root.i_uid   = 0;
    root.i_size  = 2 * 1024;   /* 2 blocks of dir entries */
    root.i_links_count = 2;    /* . and .. */
    root.i_blocks = 4;         /* 2 blocks × (1024/512) = 4 sectors */
    root.i_block[0] = DATA_START;
    root.i_block[1] = DATA_START + 1;

    /* 写 inode 2 到 inode table (block IT_START, inode offset 128 bytes) */
    {
        char it_block[1024];
        clear_block(disk, IT_START);
        blkdev_read(disk, IT_START * 2, it_block, 2);  /* 读当前内容 */
        /* 复制 root inode 到偏移 128 处(inode 2 = (2-1)*128) */
        for (int i = 0; i < (int)sizeof(root); i++)
            it_block[128 + i] = ((char *)&root)[i];
        write_block(disk, IT_START, it_block);
    }

    /* -------- root directory entries(. 和 .. 和 hello.txt) -------- */
    /* block DATA_START: . and .. */
    for (int i = 0; i < 1024; i++) buf[i] = 0;
    struct ext2_dir_entry *de;

    /* . (self, inode 2) */
    de = (struct ext2_dir_entry *)buf;
    de->inode     = ROOT_INO;
    de->rec_len   = 12;         /* roundup(8+1, 4) */
    de->name_len  = 1;
    de->file_type = 2;          /* dir */
    de->name[0]   = '.';

    /* .. (parent, inode 2 for root) */
    de = (struct ext2_dir_entry *)(buf + 12);
    de->inode     = ROOT_INO;
    de->rec_len   = 12;
    de->name_len  = 2;
    de->file_type = 2;
    de->name[0]   = '.';
    de->name[1]   = '.';

    /* hello.txt (inode 12, regular) */
    de = (struct ext2_dir_entry *)(buf + 24);
    de->inode     = HELLO_INO;
    de->rec_len   = 1024 - 24;  /* fill rest of block */
    de->name_len  = 9;
    de->file_type = 1;          /* regular file */
    de->name[0]   = 'h'; de->name[1] = 'e'; de->name[2] = 'l';
    de->name[3]   = 'l'; de->name[4] = 'o'; de->name[5] = '.';
    de->name[6]   = 't'; de->name[7] = 'x'; de->name[8] = 't';
    write_block(disk, DATA_START, buf);

    /* block DATA_START+1: empty(padding for root dir) */
    clear_block(disk, DATA_START + 1);

    /* -------- hello.txt inode(inode 12, regular file) -------- */
    struct ext2_inode hello_ino;
    for (int i = 0; i < sizeof(hello_ino); i++) ((char *)&hello_ino)[i] = 0;
    hello_ino.i_mode  = S_IFREG | 0644;
    hello_ino.i_size  = 11;     /* "Hello ext2!" = 11 bytes */
    hello_ino.i_links_count = 1;
    hello_ino.i_blocks = 2;     /* 1024/512 = 2 sectors */
    hello_ino.i_block[0] = DATA_START + 2;

    /* 写 inode 12 (offset 11 * 128 = 1408 = 1 block + 384 bytes) */
    {
        char it_block[1024];
        clear_block(disk, IT_START + 1);
        blkdev_read(disk, (IT_START + 1) * 2, it_block, 2);
        for (int i = 0; i < (int)sizeof(hello_ino); i++)
            it_block[384 + i] = ((char *)&hello_ino)[i];
        write_block(disk, IT_START + 1, it_block);
    }

    /* -------- file data: "Hello ext2!" -------- */
    for (int i = 0; i < 1024; i++) buf[i] = 0;
    const char *msg = "Hello ext2!";
    for (int i = 0; msg[i]; i++) buf[i] = msg[i];
    write_block(disk, DATA_START + 2, buf);

    serial_puts("ext2_mkfs: done\r\n");
}

/* ================================================================
 * ext2_test — 验证 ext2 读取
 * ================================================================ */
void ext2_test(struct gendisk *disk)
{
    /* 声明 ext2.c 中的函数 */
    extern int ext2_mount(struct gendisk *);
    extern unsigned int ext2_lookup(unsigned int, const char *);
    extern int ext2_read_file(unsigned int, void *, unsigned int, unsigned int);

    if (ext2_mount(disk) < 0) {
        serial_puts("ext2_test: mount failed\r\n");
        return;
    }

    /* 查找 /hello.txt */
    unsigned int ino = ext2_lookup(2, "hello.txt");
    if (!ino) {
        serial_puts("ext2_test: hello.txt not found\r\n");
        return;
    }
    serial_puts("ext2: found hello.txt, ino=");
    print_hex64(ino);
    serial_puts("\r\n");

    /* 读取内容 */
    char content[64];
    int n = ext2_read_file(ino, content, 0, sizeof(content) - 1);
    if (n > 0) {
        content[n] = '\0';
        serial_puts("ext2: content='");
        serial_puts(content);
        serial_puts("'\r\n");
    }
}
