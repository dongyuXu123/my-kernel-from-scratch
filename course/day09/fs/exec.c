/*
 * fs/exec.c — exec 系统调用实现
 *
 * 对照: reference/linux-7.1/fs/exec.c
 *
 * do_sys_exec(filename): 从 ramfs 加载 ELF, 替换当前进程
 * 成功时修改调用者栈帧, 返回到新程序入口
 */

#include "kernel.h"

/* ramfs 接口 */
extern int ramfs_read_file(const char *name, char *buf, int maxlen);

/* ELF 加载器 */
extern unsigned long elf_load_mem(void *data, unsigned int len);

#define EXEC_BUF_SIZE 65536  /* 64KB max ELF */

static char exec_buf[EXEC_BUF_SIZE];

/*
 * do_sys_exec — 由 entry.S 调用
 * 参数: filename 在 %rdi(被调用者保存, 这里直接读取)
 *
 * 返回: 成功时直接修改栈帧跳到新程序(不返回)
 *       失败返回 0
 *
 * 栈帧布局(从 entry.S 的 .do_exec 看):
 *   [rsp+0]:  return addr → .do_exec
 *   [rsp+8]:  saved %rsi
 *   [rsp+16]: saved %rdi
 *   [rsp+24]: saved %rax
 *   [rsp+32]: RIP(用户返回地址) ← 修改这里
 *   [rsp+40]: CS
 *   [rsp+48]: RFLAGS
 *   [rsp+56]: RSP(用户栈)    ← 修改这里
 *   [rsp+64]: SS
 */
unsigned long do_sys_exec(const char *filename)
{
    /* 读文件内容 */
    int n = ramfs_read_file(filename, exec_buf, EXEC_BUF_SIZE);
    if (n <= 0) {
        serial_puts("exec: file not found: ");
        serial_puts(filename);
        serial_puts("\r\n");
        return 0;
    }

    serial_puts("exec: loading ");
    serial_puts(filename);
    serial_puts(" (");
    print_hex64(n);
    serial_puts(" bytes)\r\n");

    /* 加载 ELF */
    unsigned long entry = elf_load_mem(exec_buf, (unsigned int)n);
    if (!entry) {
        serial_puts("exec: ELF load failed\r\n");
        return 0;
    }

    /*
     * 修改栈帧: 替换用户 RIP 和 RSP
     * 从 do_sys_exec 的栈帧找: return addr → .do_exec → ...
     *
     * 使用 __builtin_frame_address 或手动偏移。
     * 简化: entry.S 的 .do_exec 负责修改栈帧。
     * 这里只返回 entry point。
     */
    return entry;
}
