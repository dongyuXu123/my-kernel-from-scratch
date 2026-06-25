/*
 * fs/ext2.c — 简化 ext2 只读文件系统
 *
 * 对照: reference/linux-7.1/fs/ext2/
 *   super.c (ext2_fill_super), inode.c (ext2_get_inode, ext2_get_block),
 *   dir.c (ext2_readdir, ext2_find_entry)
 *
 * 限制:
 *   - 仅 1024 字节块(s_log_block_size=0)
 *   - 仅 revision 0(128B inode)
 *   - 仅直接块指针(i_block[0..11])
 *   - 只读, 无缓存
 *
 * 磁盘布局(start=0):
 *   offset 0:     boot block(1024B, ext2 不管)
 *   offset 1024:  superblock(1024B)
 *   block 2:      group descriptors
 *   block N:      inode table
 *   block M:      data blocks
 */

#include "kernel.h"
#include "blkdev.h"

/* ================================================================
 * on-disk 结构(对照 ext2.h:410-597)
 * ================================================================ */

/* superblock — 1024 字节, 位于 offset 1024 */
struct ext2_super_block {
    unsigned int  s_inodes_count;       /* 0x00 */
    unsigned int  s_blocks_count;       /* 0x04 */
    unsigned int  s_r_blocks_count;     /* 0x08 */
    unsigned int  s_free_blocks_count;  /* 0x0C */
    unsigned int  s_free_inodes_count;  /* 0x10 */
    unsigned int  s_first_data_block;   /* 0x14 */
    unsigned int  s_log_block_size;     /* 0x18: block=1024<<this */
    unsigned int  s_log_frag_size;      /* 0x1C */
    unsigned int  s_blocks_per_group;   /* 0x20 */
    unsigned int  s_frags_per_group;    /* 0x24 */
    unsigned int  s_inodes_per_group;   /* 0x28 */
    unsigned int  s_mtime;              /* 0x2C */
    unsigned int  s_wtime;              /* 0x30 */
    unsigned short s_mnt_count;         /* 0x34 */
    short         s_max_mnt_count;      /* 0x36 */
    unsigned short s_magic;             /* 0x38: 0xEF53 */
    /* ... more fields follow, we stop here */
};

/* group descriptor — 32 字节 */
struct ext2_group_desc {
    unsigned int  bg_block_bitmap;      /* 0x00 */
    unsigned int  bg_inode_bitmap;      /* 0x04 */
    unsigned int  bg_inode_table;       /* 0x08: 绝对块号 */
    unsigned short bg_free_blocks_count;/* 0x0C */
    unsigned short bg_free_inodes_count;/* 0x0E */
    unsigned short bg_used_dirs_count;  /* 0x10 */
    unsigned short bg_pad;              /* 0x12 */
    unsigned int  bg_reserved[3];       /* 0x14 */
};

/* inode — 128 字节(revision 0) */
struct ext2_inode {
    unsigned short i_mode;              /* 0x00: S_IFREG=0x8000, S_IFDIR=0x4000 */
    unsigned short i_uid;               /* 0x02 */
    unsigned int   i_size;              /* 0x04: 文件/目录大小 */
    unsigned int   i_atime;             /* 0x08 */
    unsigned int   i_ctime;             /* 0x0C */
    unsigned int   i_mtime;             /* 0x10 */
    unsigned int   i_dtime;             /* 0x14 */
    unsigned short i_gid;               /* 0x18 */
    unsigned short i_links_count;       /* 0x1A */
    unsigned int   i_blocks;            /* 0x1C: 512B扇区数 */
    unsigned int   i_flags;             /* 0x20 */
    unsigned int   i_osd1;              /* 0x24 */
    unsigned int   i_block[15];         /* 0x28: 块指针[0..11]直接,[12]单间接... */
    /* ... more fields follow */
};

/* 目录项 — 变长, 最小 8 字节 */
struct ext2_dir_entry {
    unsigned int   inode;               /* inode 号(0=已删除) */
    unsigned short rec_len;             /* 本条目总长(4字节对齐) */
    unsigned char  name_len;            /* 文件名长度 */
    unsigned char  file_type;           /* 0=未知, 1=regular, 2=dir */
    char           name[];              /* 文件名(不 NUL-terminated!) */
};

#define EXT2_SUPER_MAGIC  0xEF53
#define EXT2_ROOT_INO     2
#define EXT2_BLOCK_SIZE   1024
#define EXT2_INODE_SIZE   128
#define S_IFDIR  0x4000
#define S_IFREG  0x8000

/* ================================================================
 * 内部状态
 * ================================================================ */
static struct gendisk *ext2_disk;
static unsigned int ext2_block_size;
static unsigned int ext2_inodes_per_group;
static unsigned int ext2_blocks_per_group;
static unsigned int ext2_groups_count;
static struct ext2_group_desc *ext2_gdesc; /* 缓存的组描述符表 */
static char gdesc_buf[4096];               /* 组描述符缓冲区 */

/* ================================================================
 * ext2_read_block — 从块设备读一个块
 * ================================================================ */
static int ext2_read_block(unsigned int block, void *buf)
{
    return blkdev_read(ext2_disk, block * (EXT2_BLOCK_SIZE / 512),
                       buf, EXT2_BLOCK_SIZE / 512);
}

/* ================================================================
 * ext2_mount — 挂载 ext2 文件系统
 *
 * 对照: ext2_fill_super() (super.c:878)
 * ================================================================ */
int ext2_mount(struct gendisk *disk)
{
    ext2_disk = disk;
    char sb_buf[1024];

    /* 读 superblock(offset 1024, 2 sectors) */
    blkdev_read(disk, 2, sb_buf, 2);
    struct ext2_super_block *sb = (struct ext2_super_block *)sb_buf;

    if (sb->s_magic != EXT2_SUPER_MAGIC) {
        serial_puts("ext2: bad magic\r\n");
        return -1;
    }

    ext2_block_size      = EXT2_BLOCK_SIZE;  /* 简化: 固定 1024 */
    ext2_inodes_per_group = sb->s_inodes_per_group;
    ext2_blocks_per_group = sb->s_blocks_per_group;

    /* 计算组数量 */
    ext2_groups_count = ((sb->s_blocks_count - sb->s_first_data_block - 1)
                         / sb->s_blocks_per_group) + 1;

    /* 读组描述符(block 2 = offset 2048) */
    blkdev_read(disk, 4, gdesc_buf, 2);
    ext2_gdesc = (struct ext2_group_desc *)gdesc_buf;

    serial_puts("ext2: mounted, ");
    print_hex64(sb->s_blocks_count);
    serial_puts(" blocks, ");
    print_hex64(sb->s_inodes_count);
    serial_puts(" inodes\r\n");

    return 0;
}

/* ================================================================
 * ext2_read_inode — 读取指定 inode
 *
 * 对照: ext2_get_inode() (inode.c:1318)
 * ================================================================ */
int ext2_read_inode(unsigned int ino, struct ext2_inode *inode)
{
    if (ino < 1) return -1;

    /* 找属于哪个组 */
    unsigned int group = (ino - 1) / ext2_inodes_per_group;
    if (group >= ext2_groups_count) return -1;

    struct ext2_group_desc *gdp = &ext2_gdesc[group];

    /* 组内偏移 */
    unsigned int offset = ((ino - 1) % ext2_inodes_per_group) * EXT2_INODE_SIZE;

    /* inode 所在块 */
    unsigned int block = gdp->bg_inode_table + offset / EXT2_BLOCK_SIZE;

    /* 读该块 */
    char buf[EXT2_BLOCK_SIZE];
    if (ext2_read_block(block, buf) < 0) return -1;

    /* 复制 inode */
    char *src = buf + (offset % EXT2_BLOCK_SIZE);
    for (unsigned int i = 0; i < sizeof(*inode); i++)
        ((char *)inode)[i] = src[i];

    return 0;
}

/* ================================================================
 * ext2_lookup — 在目录中查找文件名
 *
 * 对照: ext2_find_entry() (dir.c:343)
 *
 * 返回 inode 号, 失败返回 0
 * ================================================================ */
unsigned int ext2_lookup(unsigned int dir_ino, const char *name)
{
    struct ext2_inode dir_inode;
    if (ext2_read_inode(dir_ino, &dir_inode) < 0) return 0;
    if ((dir_inode.i_mode & 0xF000) != S_IFDIR) return 0;

    char block_buf[EXT2_BLOCK_SIZE];
    unsigned int total_read = 0;

    /* 遍历直接块(最多 12 个) */
    for (int i = 0; i < 12 && total_read < dir_inode.i_size; i++) {
        if (dir_inode.i_block[i] == 0) continue;

        if (ext2_read_block(dir_inode.i_block[i], block_buf) < 0) continue;

        unsigned int off = 0;
        while (off < EXT2_BLOCK_SIZE && total_read < dir_inode.i_size) {
            struct ext2_dir_entry *de = (struct ext2_dir_entry *)(block_buf + off);

            if (de->rec_len == 0) break;

            if (de->inode != 0) {
                /* 检查名称匹配 */
                unsigned int len = de->name_len;
                const char *n = name;
                unsigned int matched = 1;
                for (unsigned int j = 0; j < len; j++) {
                    if (de->name[j] != n[j] || n[j] == '\0') {
                        matched = 0;
                        break;
                    }
                }
                if (matched && n[len] == '\0') {
                    return de->inode;
                }
            }

            unsigned int step = de->rec_len;
            if (step == 0) break;
            off += step;
            total_read += step;
        }
    }
    return 0;
}

/* ================================================================
 * ext2_read_file — 从 inode 读文件数据
 *
 * 对照: ext2_get_block() → 直接块 (inode.c:785)
 *
 * 只支持直接块指针(i_block[0..11])
 * ================================================================ */
int ext2_read_file(unsigned int ino, void *buf, unsigned int offset, unsigned int len)
{
    struct ext2_inode inode;
    if (ext2_read_inode(ino, &inode) < 0) return -1;
    if ((inode.i_mode & 0xF000) != S_IFREG) return -1;
    if (offset >= inode.i_size) return 0;

    unsigned int remaining = len;
    if (offset + remaining > inode.i_size)
        remaining = inode.i_size - offset;

    char *dest = (char *)buf;
    char block_buf[EXT2_BLOCK_SIZE];

    while (remaining > 0) {
        unsigned int block_idx = offset / EXT2_BLOCK_SIZE;
        unsigned int block_off = offset % EXT2_BLOCK_SIZE;

        if (block_idx >= 12 || inode.i_block[block_idx] == 0)
            break;  /* 超出直接块范围或空洞 */

        if (ext2_read_block(inode.i_block[block_idx], block_buf) < 0)
            break;

        unsigned int chunk = EXT2_BLOCK_SIZE - block_off;
        if (chunk > remaining) chunk = remaining;

        for (unsigned int i = 0; i < chunk; i++)
            dest[i] = block_buf[block_off + i];

        dest      += chunk;
        offset    += chunk;
        remaining -= chunk;
    }

    return (int)(len - remaining);
}
