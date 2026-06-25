/*
 * fs/procfs.c — /proc 伪文件系统 (Day 52)
 *
 * 对照: reference/linux-7.1/fs/proc/
 */

#include "kernel.h"

#define PROCFS_MAX 16

struct proc_entry {
    const char *name;
    void (*read_func)(char *buf, int maxlen);
};

static struct proc_entry entries[PROCFS_MAX];
static int nr_entries;

static int serial_sprintf_stub(char *buf, const char *fmt, ...) {
    unsigned long *args = (unsigned long *)(&fmt + 1);
    char *start = buf;
    while (*fmt) {
        if (*fmt == '%' && fmt[1] == 'd') {
            int v = (int)*args++; fmt += 2;
            if (v == 0) { *buf++ = '0'; continue; }
            char t[16]; int n = 0;
            while (v) { t[n++] = '0' + (v % 10); v /= 10; }
            while (n) *buf++ = t[--n];
        } else if (*fmt == '%' && fmt[1] == 's') {
            const char *s = (const char *)*args++; fmt += 2;
            while (*s) *buf++ = *s++;
        } else {
            *buf++ = *fmt++;
        }
    }
    *buf = 0;
    return (int)(buf - start);
}

static int strcmp_stub(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

/* proc_meminfo — /proc/meminfo */
static void proc_meminfo_read(char *buf, int maxlen) {
    extern unsigned long total_pages, free_page_cnt;
    char *p = buf;
    p += serial_sprintf_stub(p, "MemTotal: %d kB\r\n", (int)(total_pages * 4));
    p += serial_sprintf_stub(p, "MemFree:  %d kB\r\n", (int)(free_page_cnt * 4));
    if (p < buf + maxlen) *p = 0;
}

/* proc_cpuinfo — /proc/cpuinfo */
static void proc_cpuinfo_read(char *buf, int maxlen) {
    char *p = buf;
    p += serial_sprintf_stub(p, "processor: 0\r\n");
    p += serial_sprintf_stub(p, "vendor_id: GenuineIntel\r\n");
    p += serial_sprintf_stub(p, "model name: MyKernel CPU\r\n");
    p += serial_sprintf_stub(p, "cores: 1\r\n");
    if (p < buf + maxlen) *p = 0;
}

/* proc_self_read — /proc/self */
static void proc_self_read(char *buf, int maxlen) {
    extern struct task_struct *current_task;
    char *p = buf;
    p += serial_sprintf_stub(p, "Pid: %d\r\n", 1);
    p += serial_sprintf_stub(p, "State: running\r\n");
    if (p < buf + maxlen) *p = 0;
}

/* procfs_register — 注册 proc 文件 */
void procfs_register(const char *name, void (*fn)(char *, int)) {
    if (nr_entries < PROCFS_MAX) {
        entries[nr_entries].name = name;
        entries[nr_entries].read_func = fn;
        nr_entries++;
    }
}

/* procfs_read — 读取 proc 文件内容 */
int procfs_read(const char *name, char *buf, int maxlen) {
    for (int i = 0; i < nr_entries; i++) {
        if (strcmp_stub(entries[i].name, name) == 0) {
            entries[i].read_func(buf, maxlen);
            return 0;
        }
    }
    return -1;
}

/* procfs_init — 初始化 /proc */
void procfs_init(void) {
    nr_entries = 0;
    procfs_register("meminfo", proc_meminfo_read);
    procfs_register("cpuinfo", proc_cpuinfo_read);
    procfs_register("self",    proc_self_read);
    serial_puts("procfs: initialized (meminfo, cpuinfo, self)\r\n");
}
