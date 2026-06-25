/*
 * fs/ramfs.c — 简易内存文件系统(D 阶段)
 *
 * 对照: reference/linux-7.1/fs/ramfs/inode.c
 *
 * 设计思路(对照真实内核):
 *   真实内核的 ramfs 基于 VFS 层(super_block/inode/dentry/file),
 *   挂载在 tmpfs 上。本实现简化为:
 *     - 全局文件数组(最多 16 个文件)
 *     - 每个文件有名称(name[32])、数据指针(data)、大小(size)
 *     - 创建时分配内存(kmalloc), 写入时复制数据
 *
 * 数据结构层次(对照):
 *   super_block  → (无, 简化为全局数组)
 *   inode        → ramfs_file (名称 + 数据 + 大小)
 *   dentry       → (无, 名称存在 file 里)
 *   file         → (无, 用数组索引代替文件描述符)
 */

#include "kernel.h"
#include "mm.h"

/* -------- 常量 -------- */
#define RAMFS_MAX_FILES  16       /* 最多 16 个文件 */
#define RAMFS_NAME_MAX   32       /* 文件名最大长度 */

/* -------- 文件结构(简化版 inode) -------- */
typedef struct ramfs_file {
    char name[RAMFS_NAME_MAX];    /* 文件名 */
    char *data;                   /* 数据指针(kmalloc 分配) */
    unsigned long size;           /* 数据大小(字节) */
    int  used;                    /* 是否在使用 */
} ramfs_file_t;

/* -------- 全局文件表 -------- */
static ramfs_file_t file_table[RAMFS_MAX_FILES];

/* ================================================================
 * ramfs_init — 初始化文件系统(清空文件表)
 * ================================================================ */
void ramfs_init(void)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        file_table[i].used = 0;
        file_table[i].data = 0;
        file_table[i].size = 0;
    }
    serial_puts("[ramfs] init done\r\n");
}

/* ================================================================
 * ramfs_create(name) — 创建文件, 返回文件索引(文件描述符)
 * 对照: ramfs_create() → 真实内核返回 inode*
 * 失败返回 -1
 * ================================================================ */
int ramfs_create(const char *name)
{
    /* 找空闲槽位 */
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (file_table[i].used)
            continue;

        /* 复制文件名 */
        int j;
        for (j = 0; j < RAMFS_NAME_MAX - 1 && name[j]; j++)
            file_table[i].name[j] = name[j];
        file_table[i].name[j] = '\0';

        file_table[i].data = 0;
        file_table[i].size = 0;
        file_table[i].used = 1;
        return i;
    }
    return -1;  /* 文件表满 */
}

/* ================================================================
 * ramfs_write(fd, buf, len) — 写入文件
 * 对照: vfs_write() → ramfs 的写操作
 * 返回实际写入字节数, 失败返回 -1
 * ================================================================ */
int ramfs_write(int fd, const char *buf, int len)
{
    if (fd < 0 || fd >= RAMFS_MAX_FILES || !file_table[fd].used)
        return -1;

    /* 释放旧数据 */
    if (file_table[fd].data)
        kfree(file_table[fd].data);

    if (len <= 0) {
        file_table[fd].data = 0;
        file_table[fd].size = 0;
        return 0;
    }

    /* 使用 alloc_pages 代替 kmalloc (避免 slab 空闲链表被覆盖) */
    extern void *alloc_pages(int order);
    char *new_data = (char *)alloc_pages(0);
    if (!new_data) return -1;

    /* 复制数据 */
    int copy = (len < 4096) ? len : 4096;
    for (int i = 0; i < copy; i++)
        new_data[i] = buf[i];

    file_table[fd].data = new_data;
    file_table[fd].size = copy;
    return copy;
}

/* ================================================================
 * ramfs_read(fd, buf, len) — 读取文件
 * 对照: vfs_read()
 * 返回实际读取字节数
 * ================================================================ */
int ramfs_read(int fd, char *buf, int len)
{
    if (fd < 0 || fd >= RAMFS_MAX_FILES || !file_table[fd].used)
        return -1;
    if (!file_table[fd].data)
        return 0;

    int n = (len < (int)file_table[fd].size) ? len : (int)file_table[fd].size;
    for (int i = 0; i < n; i++)
        buf[i] = file_table[fd].data[i];
    return n;
}

/* ================================================================
 * ramfs_list — 列出所有文件(调试用)
 * 对照: readdir() / ls 命令
 * ================================================================ */
void ramfs_list(void)
{
    serial_puts("[ramfs] files:\r\n");
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!file_table[i].used)
            continue;
        serial_puts("  ");
        serial_puts(file_table[i].name);
        serial_puts(" (");
        print_hex64(file_table[i].size);
        serial_puts(" bytes)\r\n");
    }
}

/* ramfs_get_size — 获取文件大小 (供 lseek/fstat 使用) */
int ramfs_get_size(int idx)
{
    if (idx < 0 || idx >= RAMFS_MAX_FILES || !file_table[idx].used)
        return 0;
    return file_table[idx].size;
}

/* ramfs_list_names — 获取文件名列表(供 GUI 文件浏览器使用) */
int ramfs_list_names(char names[16][32])
{
    int count = 0;
    for (int i = 0; i < RAMFS_MAX_FILES && count < 16; i++) {
        if (!file_table[i].used) continue;
        char *d = names[count], *s = file_table[i].name;
        int j = 0;
        while (*s && j < 31) d[j++] = *s++;
        d[j] = 0;
        count++;
    }
    return count;
}

/* ================================================================
 * ramfs_cat — 按名称查找文件并打印内容(syscall 7)
 * ================================================================ */
void ramfs_cat(const char *name)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!file_table[i].used)
            continue;

        int match = 1;
        for (int j = 0; j < RAMFS_NAME_MAX; j++) {
            if (file_table[i].name[j] != name[j]) { match = 0; break; }
            if (name[j] == '\0') break;
        }
        if (!match) continue;

        if (file_table[i].data && file_table[i].size > 0) {
            for (unsigned long k = 0; k < file_table[i].size; k++)
                serial_putchar(file_table[i].data[k]);
        }
        return;
    }
    serial_puts("cat: file not found\r\n");
}

/* ================================================================
 * ramfs_read_file — 按名称读取文件到缓冲区(exec 用)
 * 返回实际读取字节数, 失败返回 -1
 * ================================================================ */
int ramfs_read_file(const char *name, char *buf, int maxlen)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!file_table[i].used) continue;

        int match = 1;
        for (int j = 0; j < RAMFS_NAME_MAX; j++) {
            if (file_table[i].name[j] != name[j]) { match = 0; break; }
            if (name[j] == '\0') break;
        }
        if (!match) continue;

        if (!file_table[i].data) return 0;
        int n = (int)file_table[i].size;
        if (n > maxlen) n = maxlen;
        for (int k = 0; k < n; k++) buf[k] = file_table[i].data[k];
        return n;
    }
    return -1;
}

/* ================================================================
 * ramfs_lookup — 按名称查找文件, 返回文件索引
 * ================================================================ */
int ramfs_lookup(const char *name)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!file_table[i].used) continue;
        int match = 1;
        for (int j = 0; j < RAMFS_NAME_MAX; j++) {
            if (file_table[i].name[j] != name[j]) { match = 0; break; }
            if (name[j] == '\0') break;
        }
        if (match) return i;
    }
    return -1;
}

/* ================================================================
 * ramfs_fd_read — 按文件索引+位置读取
 * ================================================================ */
int ramfs_fd_read(int idx, int pos, char *buf, int len)
{
    if (idx < 0 || idx >= RAMFS_MAX_FILES || !file_table[idx].used) return -1;
    if (!file_table[idx].data) return 0;

    int remain = (int)file_table[idx].size - pos;
    if (remain <= 0) return 0;
    if (len > remain) len = remain;

    for (int i = 0; i < len; i++)
        buf[i] = file_table[idx].data[pos + i];
    return len;
}

/* ================================================================
 * ramfs_fd_write — 按文件索引+位置写入
 * ================================================================ */
int ramfs_fd_write(int idx, int pos, const char *buf, int len)
{
    if (idx < 0 || idx >= RAMFS_MAX_FILES || !file_table[idx].used) return -1;

    /* 扩展文件 */
    int new_size = pos + len;
    if (new_size > (int)file_table[idx].size) {
        /* 分配新缓冲区 */
        char *new_data = (char *)kmalloc(new_size);
        if (!new_data) return -1;
        /* 复制旧数据 */
        for (unsigned long i = 0; i < file_table[idx].size; i++)
            new_data[i] = file_table[idx].data[i];
        if (file_table[idx].data) kfree(file_table[idx].data);
        file_table[idx].data = new_data;
        file_table[idx].size = new_size;
    }

    for (int i = 0; i < len; i++)
        file_table[idx].data[pos + i] = buf[i];
    return len;
}

/* ================================================================
 * ramfs_namei — VFS 接口: 按名称查找/创建 inode
 * ================================================================ */
#include "vfs.h"

struct inode *ramfs_namei(const char *name)
{
    extern int ramfs_lookup(const char *name);
    int idx = ramfs_lookup(name);
    if (idx < 0) return 0;

    /* 创建 inode(每文件静态分配) */
    static struct inode ino_table[16];
    static struct file_operations ramfs_fops;

    /* 已存在? */
    for (int i = 0; i < 16; i++)
        if (ino_table[i].i_ino == idx + 1) return &ino_table[i];

    /* 新 inode */
    for (int i = 0; i < 16; i++) {
        if (ino_table[i].i_ino == 0) {
            ino_table[i].i_ino = idx + 1;
            ino_table[i].i_private = (void *)(long)idx;
            ino_table[i].i_fop = &ramfs_fops;
            return &ino_table[i];
        }
    }
    return 0;
}

/* ---- VFS file_operations for ramfs ---- */
#include "vfs.h"

static int ramfs_fop_read(struct file *f, char *buf, int len) {
    int idx = (int)(long)f->f_inode->i_private;
    if (idx < 0 || idx >= 16) return -1;
    extern int ramfs_fd_read(int idx, int pos, char *buf, int len);
    return ramfs_fd_read(idx, f->f_pos, buf, len);
}
static int ramfs_fop_write(struct file *f, const char *buf, int len) {
    int idx = (int)(long)f->f_inode->i_private;
    if (idx < 0 || idx >= 16) return -1;
    extern int ramfs_fd_write(int idx, int pos, const char *buf, int len);
    return ramfs_fd_write(idx, f->f_pos, buf, len);
}
static struct file_operations ramfs_fops = { .read = ramfs_fop_read, .write = ramfs_fop_write };

/* ================================================================
 * sys_writeto — syscall 10: 写数据到文件(创建或覆盖)
 * ================================================================ */
int sys_writeto(const char *name, const char *data, int len)
{
    /* 查找或创建文件 */
    int idx = ramfs_lookup(name);
    if (idx < 0) {
        idx = ramfs_create(name);
        if (idx < 0) return -1;
    }
    return ramfs_write(idx, data, len);
}
