/*
 * kernel/signal.c — 信号处理
 *
 * 对照: reference/linux-7.1/kernel/signal.c
 */
#include "kernel.h"
#include "sched.h"

#define SIGINT  2
#define SIGKILL 9
#define NSIG   16

static void (*sig_handlers[NSIG])(int);

/* 当前前台进程的 pending signal */
static int pending_sig;
static int pending_pid;

void signal_init(void) {
    for (int i = 0; i < NSIG; i++) sig_handlers[i] = 0;
    pending_sig = 0;
}

int sys_signal(int sig, void (*handler)(int)) {
    if (sig < 0 || sig >= NSIG) return -1;
    sig_handlers[sig] = handler;
    serial_puts("signal: registered handler for sig ");
    print_hex64(sig); serial_puts("\r\n");
    return 0;
}

int sys_kill(int pid, int sig) {
    if (sig < 0 || sig >= NSIG) return -1;
    pending_sig = sig;
    pending_pid = pid;
    serial_puts("signal: sent sig "); print_hex64(sig);
    serial_puts(" to pid "); print_hex64(pid); serial_puts("\r\n");
    return 0;
}

/* 在中断返回前调用: 检查并分发 pending signals */
void signal_deliver(void) {
    if (pending_sig > 0 && sig_handlers[pending_sig]) {
        sig_handlers[pending_sig](pending_sig);
        pending_sig = 0;
    }
}

/* Ctrl-C 检测: 由键盘输入处理调用 */
void signal_check_ctrlc(char c) {
    if (c == 0x03) {  /* Ctrl-C */
        serial_puts("^C\r\n");
        pending_sig = SIGINT;
    }
}
