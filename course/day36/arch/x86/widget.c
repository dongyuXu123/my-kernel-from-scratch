/* arch/x86/widget.c — GUI 控件 (Day 33 GUI-3)
 * 按钮、标签、文本框控件
 */
#include "kernel.h"

#define MAX_WIDGETS 64
#define BTN_H 28
#define INPUT_H 24

enum { WIDGET_BUTTON, WIDGET_LABEL, WIDGET_INPUT };

struct widget {
    int type;
    int x, y, w, h;
    const char *text;
    char input_buf[64];
    int input_len;
    int visible;
    unsigned int bg;
    unsigned int fg;
    void (*on_click)(int id);
    int id;
};

static struct widget widgets[MAX_WIDGETS];
static int nr_widgets;
static int focus_widget = -1;

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

/* 画凸起/凹陷边框 */
static void draw_3d_rect(int x, int y, int w, int h, int raised) {
    unsigned int lt = raised ? 0x00FFFFFF : 0x00555555;
    unsigned int rb = raised ? 0x00555555 : 0x00FFFFFF;
    fill_rect(x, y, w, 1, lt);
    fill_rect(x, y, 1, h, lt);
    fill_rect(x + w - 1, y, 1, h, rb);
    fill_rect(x, y + h - 1, w, 1, rb);
}

void widget_draw_all(void) {
    for (int i = 0; i < nr_widgets; i++) {
        struct widget *w = &widgets[i];
        if (!w->visible) continue;
        
        switch (w->type) {
        case WIDGET_BUTTON:
            fill_rect(w->x, w->y, w->w, w->h, w->bg);
            draw_3d_rect(w->x, w->y, w->w, w->h, 1);
            if (w->text) fb_draw_text(w->x + 8, w->y + 6, w->text, w->fg);
            break;
        case WIDGET_LABEL:
            if (w->text) fb_draw_text(w->x, w->y, w->text, w->fg);
            break;
        case WIDGET_INPUT:
            fill_rect(w->x, w->y, w->w, w->h, 0x00222222);
            draw_3d_rect(w->x, w->y, w->w, w->h, 0);
            if (w->input_len > 0)
                fb_draw_text(w->x + 4, w->y + 5, w->input_buf, w->fg);
            /* 光标 */
            if (i == focus_widget)
                fill_rect(w->x + 4 + w->input_len * 8, w->y + 4,
                          2, INPUT_H - 8, 0x00FFFFFF);
            break;
        }
    }
}

int widget_create_button(int x, int y, int w, const char *text, unsigned int bg, unsigned int fg, void (*cb)(int)) {
    if (nr_widgets >= MAX_WIDGETS) return -1;
    struct widget *wi = &widgets[nr_widgets];
    wi->type = WIDGET_BUTTON; wi->x = x; wi->y = y;
    wi->w = w; wi->h = BTN_H; wi->text = text;
    wi->bg = bg; wi->fg = fg; wi->visible = 1;
    wi->on_click = cb; wi->id = nr_widgets;
    wi->input_len = 0;
    return nr_widgets++;
}

int widget_create_label(int x, int y, const char *text, unsigned int fg) {
    if (nr_widgets >= MAX_WIDGETS) return -1;
    struct widget *wi = &widgets[nr_widgets];
    wi->type = WIDGET_LABEL; wi->x = x; wi->y = y;
    wi->w = 0; wi->h = 16; wi->text = text;
    wi->bg = 0; wi->fg = fg; wi->visible = 1;
    wi->on_click = 0; wi->id = nr_widgets;
    wi->input_len = 0;
    return nr_widgets++;
}

int widget_create_input(int x, int y, int w) {
    if (nr_widgets >= MAX_WIDGETS) return -1;
    struct widget *wi = &widgets[nr_widgets];
    wi->type = WIDGET_INPUT; wi->x = x; wi->y = y;
    wi->w = w; wi->h = INPUT_H; wi->text = 0;
    wi->bg = 0; wi->fg = 0x00FFFFFF; wi->visible = 1;
    wi->on_click = 0; wi->id = nr_widgets;
    wi->input_len = 0;
    for (int i = 0; i < 64; i++) wi->input_buf[i] = 0;
    return nr_widgets++;
}

/* 处理鼠标点击 */
int widget_handle_click(int mx, int my) {
    for (int i = nr_widgets - 1; i >= 0; i--) {
        struct widget *w = &widgets[i];
        if (!w->visible) continue;
        if (mx >= w->x && mx < w->x + w->w &&
            my >= w->y && my < w->y + w->h) {
            if (w->type == WIDGET_INPUT) {
                focus_widget = i;
            } else {
                focus_widget = -1;
            }
            if (w->on_click) w->on_click(w->id);
            return i;
        }
    }
    focus_widget = -1;
    return -1;
}

/* 键盘输入到焦点控件 */
void widget_handle_key(char c) {
    if (focus_widget < 0 || focus_widget >= nr_widgets) return;
    struct widget *w = &widgets[focus_widget];
    if (w->type != WIDGET_INPUT) return;
    
    if (c == '\b' || c == 127) {
        if (w->input_len > 0) w->input_buf[--w->input_len] = 0;
    } else if (c >= 32 && c <= 126 && w->input_len < 62) {
        w->input_buf[w->input_len++] = c;
        w->input_buf[w->input_len] = 0;
    }
}

const char *widget_get_text(int id) {
    if (id < 0 || id >= nr_widgets) return "";
    return widgets[id].input_buf;
}

void widget_set_text(int id, const char *s) {
    if (id < 0 || id >= nr_widgets || !s) return;
    widgets[id].input_len = 0;
    while (*s && widgets[id].input_len < 62)
        widgets[id].input_buf[widgets[id].input_len++] = *s++;
    widgets[id].input_buf[widgets[id].input_len] = 0;
}
