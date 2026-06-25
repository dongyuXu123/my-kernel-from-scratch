/*
 * arch/x86/desktop.c — 桌面环境
 *
 * 对照: (无直接对照 — Desktop Metaphor, Xerox PARC 1970s)
 */
/*
 * arch/x86/desktop.c — 桌面环境
 *
 * 对照: (无直接对照 — Desktop Metaphor, Xerox PARC 1970s)
 */
 */
#include "kernel.h"

#define TASKBAR_H 32
#define MAX_LAUNCHERS 8

/* 全局时钟变量 */
static int gui_tick_count = 0;
static int gui_hours = 0, gui_minutes = 0, gui_seconds = 0;

extern unsigned int *fb;
extern int fb_width, fb_height, fb_pitch;
extern void fb_draw_text(int x, int y, const char *s, unsigned int color);

static void fill_rect(int x, int y, int w, int h, unsigned int color) {
    if (!fb) return;
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx, py = y + dy;
            if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
                fb[py * fb_pitch + px] = color;
        }
}

/* 将整数转为字符串 */
static void int_to_str(int n, char *buf) {
    int i = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[16];
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = 0;
}

/* 两字符数字填充 */
static void pad2(int n, char *buf) {
    buf[0] = '0' + (n / 10) % 10;
    buf[1] = '0' + n % 10;
    buf[2] = 0;
}

struct launcher {
    const char *name;
    void (*launch)(void);
    int x, w;
};

static struct launcher launchers[MAX_LAUNCHERS];
static int nr_launchers = 0;

void desktop_add_launcher(const char *name, void (*fn)(void)) {
    if (nr_launchers >= MAX_LAUNCHERS) return;
    launchers[nr_launchers].name = name;
    launchers[nr_launchers].launch = fn;
    launchers[nr_launchers].x = 4 + nr_launchers * 72;
    launchers[nr_launchers].w = 68;
    nr_launchers++;
}

/* 检查点击启动器 */
void *desktop_check_launcher(int mx, int my) {
    int bar_y = fb_height - TASKBAR_H;
    if (my < bar_y) return 0;
    for (int i = 0; i < nr_launchers; i++) {
        if (mx >= launchers[i].x && mx < launchers[i].x + launchers[i].w)
            return (void *)launchers[i].launch;
    }
    return 0;
}

void desktop_draw(void) {
    if (!fb) return;
    int bar_y = fb_height - TASKBAR_H;
    
    /* 任务栏背景 */
    fill_rect(0, bar_y, fb_width, TASKBAR_H, 0x00101030);
    /* 顶部分割线 */
    fill_rect(0, bar_y, fb_width, 1, 0x00404060);
    
    /* 启动器按钮 */
    for (int i = 0; i < nr_launchers; i++) {
        int lx = launchers[i].x;
        fill_rect(lx, bar_y + 3, 68, TASKBAR_H - 6, 0x00202040);
        fb_draw_text(lx + 4, bar_y + 8, launchers[i].name, 0x00FFFFFF);
    }
    
    /* 时钟 (右对齐) */
    char time_str[16];
    pad2(gui_hours, time_str);
    time_str[2] = ':';
    pad2(gui_minutes, time_str + 3);
    time_str[5] = ':';
    pad2(gui_seconds, time_str + 6);
    time_str[8] = 0;
    fb_draw_text(fb_width - 80, bar_y + 8, time_str, 0x00FFFFFF);
}

/* 由定时器中断调用，更新时钟 */
void desktop_tick(void) {
    gui_tick_count++;
    if (gui_tick_count >= 100) {  /* 100Hz → 1秒 */
        gui_tick_count = 0;
        gui_seconds++;
        if (gui_seconds >= 60) {
            gui_seconds = 0; gui_minutes++;
            if (gui_minutes >= 60) {
                gui_minutes = 0; gui_hours++;
                if (gui_hours >= 24) gui_hours = 0;
            }
        }
    }
}

void desktop_init(void) {
    nr_launchers = 0;
    gui_tick_count = 0;
    gui_hours = 10;  /* 默认上午10:00 */
    gui_minutes = 0;
    gui_seconds = 0;
    serial_puts("desktop: initialized\r\n");
}
