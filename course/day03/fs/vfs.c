/*
 * fs/vfs.c — VFS 实现 + 文件描述符表
 *
 * 对照: reference/linux-7.1/include/linux/fs.h (struct file_operations)
 *       reference/linux-7.1/fs/open.c (sys_open)
 */

#include "kernel.h"
#include "vfs.h"

struct file *fd_table[MAX_FD];

void vfs_init(void)
{
    for (int i = 0; i < MAX_FD; i++)
        fd_table[i] = 0;
    /* stdin/stdout 设为串口(特殊标记) */
    fd_table[0] = (struct file *)1;  /* stdin: 串口读 */
    fd_table[1] = (struct file *)2;  /* stdout: 串口写 */
}

int vfs_open(const char *name)
{
    /* 调用 ramfs 查找/创建 inode */
    extern struct inode *ramfs_namei(const char *name);
    struct inode *in = ramfs_namei(name);
    if (!in) return -1;

    /* 分配 file 结构 */
    static struct file f_table[MAX_FD];
    for (int i = 0; i < MAX_FD; i++) {
        if (f_table[i].f_inode == 0) {
            f_table[i].f_inode = in;
            f_table[i].f_pos = 0;
            /* 找空闲 fd */
            for (int j = 2; j < MAX_FD; j++) {
                if (!fd_table[j]) {
                    fd_table[j] = &f_table[i];
                    return j;
                }
            }
            f_table[i].f_inode = 0;
            return -1;
        }
    }
    return -1;
}

int vfs_read(int fd, char *buf, int len)
{
    if (fd < 0 || fd >= MAX_FD) return -1;
    struct file *f = fd_table[fd];
    if (!f) return -1;

    /* 串口(stdin) */
    if (f == (struct file *)1) return 0;  /* 标记串口, 由 entry.S 处理 */

    if (!f->f_inode || !f->f_inode->i_fop || !f->f_inode->i_fop->read)
        return -1;
    int n = f->f_inode->i_fop->read(f, buf, len);
    if (n > 0) f->f_pos += n;
    return n;
}

int vfs_write(int fd, const char *buf, int len)
{
    if (fd < 0 || fd >= MAX_FD) return -1;
    struct file *f = fd_table[fd];
    if (!f) return -1;

    /* 串口(stdout) */
    if (f == (struct file *)2) return 0;  /* 标记串口, 由 entry.S 处理 */

    if (!f->f_inode || !f->f_inode->i_fop || !f->f_inode->i_fop->write)
        return -1;
    int n = f->f_inode->i_fop->write(f, buf, len);
    if (n > 0) f->f_pos += n;
    return n;
}
