/*
 * kernel/fd.c — 文件描述符 + Pipe
 *
 * 对照: reference/linux-7.1/fs/file.c (fd table)
 *       reference/linux-7.1/fs/pipe.c (pipe)
 */
#include "kernel.h"
#define MAX_FD 16
struct fd_entry { int file_idx; int pos; };
static struct fd_entry fd_table[MAX_FD];
struct pipe { char buf[4096]; int head, tail, count; };
static struct pipe pipes[4]; static int nr_pipe;

void fd_init(void) {
    for (int i = 0; i < MAX_FD; i++) fd_table[i].file_idx = -3;
    fd_table[0].file_idx = -1; fd_table[1].file_idx = -2;
}

int sys_open(const char *name) {
    extern int ramfs_lookup(const char*);
    int idx = ramfs_lookup(name); if (idx < 0) return -1;
    for (int i = 2; i < MAX_FD; i++)
        if (fd_table[i].file_idx == -3) { fd_table[i].file_idx=idx; fd_table[i].pos=0; return i; }
    return -1;
}

int sys_read(int fd, char *buf, int len) {
    if (fd<0||fd>=MAX_FD||fd_table[fd].file_idx==-3) return -1;
    if (fd==0) return 0;
    if (fd_table[fd].file_idx==-4) {
        struct pipe *p=&pipes[fd_table[fd].pos]; int n=0;
        while (n<len&&p->count>0) { buf[n++]=p->buf[p->tail]; p->tail=(p->tail+1)%4096; p->count--; }
        return n;
    }
    extern int ramfs_fd_read(int,int,char*,int);
    int n=ramfs_fd_read(fd_table[fd].file_idx,fd_table[fd].pos,buf,len);
    if(n>0)fd_table[fd].pos+=n; return n;
}

int sys_write(int fd, const char *buf, int len) {
    if (fd<0||fd>=MAX_FD||fd_table[fd].file_idx==-3) return -1;
    if (fd==1) return 0;
    if (fd_table[fd].file_idx==-5) {
        struct pipe *p=&pipes[fd_table[fd].pos]; int n=0;
        while (n<len&&p->count<4096) { p->buf[p->head]=buf[n++]; p->head=(p->head+1)%4096; p->count++; }
        return n;
    }
    extern int ramfs_fd_write(int,int,const char*,int);
    int n=ramfs_fd_write(fd_table[fd].file_idx,fd_table[fd].pos,buf,len);
    if(n>0)fd_table[fd].pos+=n; return n;
}

int sys_pipe(int fds[2]) {
    if(nr_pipe>=4) return -1;
    struct pipe *p=&pipes[nr_pipe]; p->head=p->tail=p->count=0;
    int r=-1,w=-1;
    for(int i=2;i<MAX_FD;i++) if(fd_table[i].file_idx==-3){if(r<0)r=i;else if(w<0){w=i;break;}}
    if(w<0) return -1;
    fd_table[r].file_idx=-4; fd_table[r].pos=nr_pipe;
    fd_table[w].file_idx=-5; fd_table[w].pos=nr_pipe;
    fds[0]=r; fds[1]=w; nr_pipe++; return 0;
}

int sys_dup2(int oldfd, int newfd) {
    if(oldfd<0||oldfd>=MAX_FD||newfd<0||newfd>=MAX_FD) return -1;
    if(fd_table[oldfd].file_idx==-3) return -1;
    fd_table[newfd]=fd_table[oldfd]; return newfd;
}

/* ================================================================
 * sys_close — 释放文件描述符 (Day 45)
 *
 * 对照: reference/linux-7.1/fs/file.c (__close_fd)
 * ================================================================ */
/* 前向声明 */
extern int ramfs_get_size(int idx);

int sys_close(int fd) {
    if (fd < 0 || fd >= MAX_FD) return -1;
    fd_table[fd].file_idx = -3;   /* -3 = unused */
    fd_table[fd].pos = 0;
    return 0;
}

/* ================================================================
 * sys_lseek — 设置文件读写位置 (Day 45)
 *
 * 对照: reference/linux-7.1/fs/read_write.c (ksys_lseek)
 * ================================================================ */
int sys_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_FD) return -1;
    if (fd_table[fd].file_idx == -3) return -1;
    
    switch (whence) {
    case 0: fd_table[fd].pos = offset; break;           /* SEEK_SET */
    case 1: fd_table[fd].pos += offset; break;          /* SEEK_CUR */
    case 2: fd_table[fd].pos = (int)ramfs_get_size(fd); break; /* SEEK_END */
            fd_table[fd].pos += offset; break;
    default: return -1;
    }
    if (fd_table[fd].pos < 0) fd_table[fd].pos = 0;
    return fd_table[fd].pos;
}

/* ================================================================
 * sys_fstat — 简化 stat: 返回文件大小 (Day 45)
 * buf[0]=size, buf[1]=type
 *
 * 对照: reference/linux-7.1/fs/stat.c (vfs_stat)
 * ================================================================ */
int sys_fstat(int fd, unsigned int *buf) {
    if (fd < 0 || fd >= MAX_FD) return -1;
    if (fd_table[fd].file_idx < 0) {
        /* 串口/pipe: type=0(字符设备), size=0 */
        if (buf) { buf[0] = 0; buf[1] = 0; }
        return 0;
    }
    if (buf) {
        buf[0] = (unsigned int)ramfs_get_size(fd_table[fd].file_idx);
        buf[1] = 1;  /* type=1(普通文件) */
    }
    return 0;
}
