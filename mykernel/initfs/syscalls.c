/*
 * initfs/syscalls.c — 用户态 syscall 包装
 *
 * 对照: (用户态包装 — Linux x86-64 syscall ABI)
 */
 * initfs/syscalls.c — syscall 包装函数 (Day 57)
 *
 * 为 newlib/用户态 C 程序提供 Linux-like syscall 接口
 */

/* write(fd, buf, len) → syscall 4 */
int write(int fd, const char *buf, int len) {
    int ret;
    __asm__ volatile (
        "movq $4, %%rax\n"
        "int  $0x80\n"
        "movl %%eax, %0"
        : "=r"(ret)
        : "D"(fd), "S"(buf), "d"(len)
        : "rax"
    );
    return ret;
}

/* read(fd, buf, len) → syscall 0 */
int read(int fd, char *buf, int len) {
    int ret;
    __asm__ volatile (
        "movq $0, %%rax\n"
        "int  $0x80\n"
        "movl %%eax, %0"
        : "=r"(ret)
        : "D"(fd), "S"(buf), "d"(len)
        : "rax"
    );
    return ret;
}

/* open(name) → syscall 3 */
int open(const char *name) {
    int ret;
    __asm__ volatile (
        "movq $3, %%rax\n"
        "int  $0x80\n"
        "movl %%eax, %0"
        : "=r"(ret)
        : "D"(name)
        : "rax"
    );
    return ret;
}

/* close(fd) → syscall 15 */
int close(int fd) {
    int ret;
    __asm__ volatile (
        "movq $15, %%rax\n"
        "int  $0x80\n"
        "movl %%eax, %0"
        : "=r"(ret)
        : "D"(fd)
        : "rax"
    );
    return ret;
}

/* fork() → syscall 1 */
int fork(void) {
    int ret;
    __asm__ volatile (
        "movq $1, %%rax\n"
        "int  $0x80\n"
        "movl %%eax, %0"
        : "=r"(ret)
        : 
        : "rax"
    );
    return ret;
}

/* exit(status) → syscall 60 */
void _exit(int status) {
    __asm__ volatile (
        "movq $60, %%rax\n"
        "int  $0x80"
        :
        : "D"(status)
        : "rax"
    );
    while (1);
}

/* getpid() → syscall 18 */
int getpid(void) {
    int ret;
    __asm__ volatile (
        "movq $18, %%rax\n"
        "int  $0x80\n"
        "movl %%eax, %0"
        : "=r"(ret)
        :
        : "rax"
    );
    return ret;
}
