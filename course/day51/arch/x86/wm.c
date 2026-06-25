/* arch/x86/wm.c — 窗口管理器 (Day 32 GUI-2) 
 * 参考: tinywm 的 move/resize/raise 概念
 * 直接在 framebuffer 上绘制窗口，无 X11
 */
#include "kernel.h"

#define MAX_WINDOWS 8
#define WIN_BORDER 3
#define WIN_TITLEBAR 20

struct window {
    int x, y, w, h;
    int visible;
    const char *title;
    unsigned int bg_color;
};

static struct window windows[MAX_WINDOWS];
static int nr_windows;
static struct window *active_win;
static struct window *drag_win;
static int drag_off_x, drag_off_y;

extern unsigned int *fb;
extern int fb_width, fb_height, fb_pitch;

/* 画矩形 */
static void fill_rect(int x, int y, int w, int h, unsigned int color) {
    if (!fb) return;
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx, py = y + dy;
            if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
                fb[py * fb_pitch + px] = color;
        }
}

/* 画字符串 (使用 fb.c 的字体) */
extern void fb_draw_text(int x, int y, const char *s, unsigned int color);

/* 绘制单个窗口 */
static void win_draw(struct window *w) {
    if (!w->visible) return;
    /* 背景 */
    fill_rect(w->x, w->y, w->w, w->h, w->bg_color);
    /* 标题栏 */
    fill_rect(w->x, w->y, w->w, WIN_TITLEBAR, 0x00000080);
    /* 边框 */
    fill_rect(w->x, w->y, w->w, WIN_BORDER, 0x00CCCCCC);
    fill_rect(w->x, w->y + w->h - WIN_BORDER, w->w, WIN_BORDER, 0x00CCCCCC);
    fill_rect(w->x, w->y, WIN_BORDER, w->h, 0x00CCCCCC);
    fill_rect(w->x + w->w - WIN_BORDER, w->y, WIN_BORDER, w->h, 0x00CCCCCC);
    /* 标题文字 */
    if (w->title) fb_draw_text(w->x + 5, w->y + 3, w->title, 0x00FFFFFF);
}

void wm_init(void) {
    nr_windows = 0; active_win = 0; drag_win = 0;
    serial_puts("wm: initialized\r\n");
}

int wm_create_window(int x, int y, int w, int h, const char *title) {
    if (nr_windows >= MAX_WINDOWS) return -1;
    struct window *win = &windows[nr_windows];
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->visible = 1; win->title = title;
    win->bg_color = 0x00404040;
    active_win = win;
    win_draw(win);
    return nr_windows++;
}

/* 检查鼠标点击: 标题栏拖拽移动窗口 */
void wm_handle_click(int mx, int my, int button) {
    /* 找最上层窗口 */
    for (int i = nr_windows - 1; i >= 0; i--) {
        struct window *w = &windows[i];
        if (!w->visible) continue;
        if (mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + w->h) {
            active_win = w;
            /* 标题栏: 开始拖拽 */
            if (my < w->y + WIN_TITLEBAR && button == 1) {
                drag_win = w;
                drag_off_x = mx - w->x;
                drag_off_y = my - w->y;
            }
            return;
        }
    }
}

void wm_handle_move(int mx, int my) {
    if (drag_win) {
        /* 擦除旧位置 */
        fill_rect(drag_win->x - WIN_BORDER, drag_win->y - WIN_BORDER,
                  drag_win->w + WIN_BORDER*2, drag_win->h + WIN_BORDER*2, 0x00000000);
        /* 更新位置 */
        drag_win->x = mx - drag_off_x;
        drag_win->y = my - drag_off_y;
        win_draw(drag_win);
    }
}

void wm_handle_release(void) {
    drag_win = 0;
}

void wm_redraw_all(void) {
    for (int i = 0; i < nr_windows; i++) win_draw(&windows[i]);
}
