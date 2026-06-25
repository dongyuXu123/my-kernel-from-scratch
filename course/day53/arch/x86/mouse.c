/* arch/x86/mouse.c — PS/2 鼠标驱动 (Day 31 GUI-1)
 * 参考: tinywm (github.com/mackstann/tinywm) 的 X11 事件处理概念
 * 端口: 0x60=数据, 0x64=状态/命令
 * 包格式: byte1(buttons|dxsign|dysign|dxhi|dyhi), byte2(dx), byte3(dy)
 */
#include "kernel.h"

static int mouse_x = 512, mouse_y = 384;  /* 屏幕中心 */
static int mouse_buttons;
static unsigned char mouse_cycle;
static unsigned char mouse_bytes[3];
extern int fb_width, fb_height;  /* 用于边界裁剪 */

/* 等待 PS/2 控制器就绪 */
static void mouse_wait(unsigned char type) {
    int timeout = 100000;
    if (type == 0) { while (timeout-- && !(inb(0x64) & 1)); }  /* 等数据 */
    else           { while (timeout-- && (inb(0x64) & 2));  }  /* 等空闲 */
}

void mouse_init(void) {
    /* 启用鼠标 */
    mouse_wait(1); outb(0x64, 0xA8);  /* 启用辅助设备 */
    mouse_wait(1); outb(0x64, 0x20);  /* 读配置 */
    mouse_wait(0); unsigned char cfg = inb(0x60);
    mouse_wait(1); outb(0x64, 0x60);  /* 写配置 */
    mouse_wait(1); outb(0x60, cfg | 2); /* 启用鼠标中断+时钟 */
    /* 启用数据报告 */
    mouse_wait(1); outb(0x64, 0xD4);
    mouse_wait(1); outb(0x60, 0xF4);
    mouse_wait(0); inb(0x60);  /* 读 ACK */
    serial_puts("mouse: initialized\r\n");
}

void mouse_poll(void) {
    unsigned char status = inb(0x64);
    if (!(status & 1)) return;  /* 无数据 */
    if (!(status & 0x20)) return; /* 非鼠标数据 */
    
    unsigned char data = inb(0x60);
    
    switch (mouse_cycle) {
    case 0:
        if (!(data & 0x08)) break;  /* bit3 必须是 1 */
        mouse_bytes[0] = data;
        mouse_cycle++;
        break;
    case 1:
        mouse_bytes[1] = data;
        mouse_cycle++;
        break;
    case 2:
        mouse_bytes[2] = data;
        mouse_cycle = 0;
        /* 解析 */
        int dx = mouse_bytes[1] - ((mouse_bytes[0] << 4) & 0x100);
        int dy = mouse_bytes[2] - ((mouse_bytes[0] << 3) & 0x100);
        mouse_x += dx; mouse_y -= dy;  /* Y 轴反转 */
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > fb_width - 1) mouse_x = fb_width - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > fb_height - 1) mouse_y = fb_height - 1;
        mouse_buttons = mouse_bytes[0] & 0x07;
        break;
    }
}

int mouse_get_x(void) { return mouse_x; }
int mouse_get_y(void) { return mouse_y; }
int mouse_get_buttons(void) { return mouse_buttons; }

/* 绘制鼠标光标 (简单箭头, 在 framebuffer 上) */
extern unsigned int *fb;
extern int fb_pitch, fb_width, fb_height;
void mouse_draw(void) {
    if (!fb) return;
    /* 白色箭头: 8x12 */
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 8; x++) {
            int px = mouse_x + x, py = mouse_y + y;
            if (px < fb_width && py < fb_height && (x < 3 || y < 2 || (x+y) < 10))
                fb[py * fb_pitch + px] = 0x00FFFFFF;
        }
    }
}
