/*
 * kernel/ipc.c — 微内核 IPC 消息传递 (Day 58)
 *
 * 对照: reference/linux-7.1/kernel/ipc/msg.c (System V 消息队列)
 *
 * 简化: send(pid, msg, len) / recv(pid, buf, len) / reply(pid, msg, len)
 */

#include "kernel.h"

#define IPC_MSG_SIZE 256
#define IPC_MAX_PENDING 8

struct ipc_msg {
    int sender;
    char data[IPC_MSG_SIZE];
    int len;
};

static struct ipc_msg pending[IPC_MAX_PENDING];
static int pending_count;

/* ipc_send — 发送消息到目标进程 */
int ipc_send(int dst_pid, const char *msg, int len)
{
    if (pending_count >= IPC_MAX_PENDING) return -1;
    if (len > IPC_MSG_SIZE) len = IPC_MSG_SIZE;

    pending[pending_count].sender = 1;  /* 简化: 当前 PID=1 */
    for (int i = 0; i < len; i++)
        pending[pending_count].data[i] = msg[i];
    pending[pending_count].len = len;
    pending_count++;

    serial_puts("IPC: send to pid=");
    print_hex64(dst_pid);
    serial_puts(" len=");
    print_hex64(len);
    serial_puts("\r\n");
    return len;
}

/* ipc_recv — 接收消息 */
int ipc_recv(int src_pid, char *buf, int maxlen)
{
    if (pending_count == 0) return 0;
    (void)src_pid;  /* 简化: 接收任何发送者 */

    int len = pending[0].len;
    if (len > maxlen) len = maxlen;
    for (int i = 0; i < len; i++) buf[i] = pending[0].data[i];

    /* 移除已消费的消息 */
    for (int i = 1; i < pending_count; i++)
        pending[i - 1] = pending[i];
    pending_count--;

    return len;
}

/* ipc_init — 初始化 IPC 子系统 */
void ipc_init(void)
{
    pending_count = 0;
    serial_puts("IPC: message passing initialized\r\n");
}
