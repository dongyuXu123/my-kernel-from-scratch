/*
 * include/vfs.h — 虚拟文件系统(VFS)抽象层
 *
 * 对照: reference/linux-7.1/include/linux/fs.h
 */

#ifndef _VFS_H
#define _VFS_H

struct file;
struct inode;

/* ---- 文件操作 ---- */
struct file_operations {
    int (*read)(struct file *f, char *buf, int len);
    int (*write)(struct file *f, const char *buf, int len);
};

/* ---- inode ---- */
struct inode {
    int i_ino;
    int i_size;
    void *i_private;
    struct file_operations *i_fop;
};

/* ---- 文件 ---- */
struct file {
    struct inode *f_inode;
    int f_pos;
    int f_flags;
};

/* ---- 文件描述符表 ---- */
#define MAX_FD 16
extern struct file *fd_table[MAX_FD];

extern void vfs_init(void);
extern int  vfs_open(const char *name);
extern int  vfs_read(int fd, char *buf, int len);
extern int  vfs_write(int fd, const char *buf, int len);

#endif
