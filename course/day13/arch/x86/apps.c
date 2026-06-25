/*
 * arch/x86/apps.c — GUI 应用程序
 *
 * 对照: (无直接对照 — MVC Pattern, Windows Calculator/Notepad)
 */
/*
 * arch/x86/apps.c — GUI 应用程序
 *
 * 对照: (无直接对照 — MVC Pattern, Windows Calculator/Notepad)
 */
 */
#include "kernel.h"

/* ---------- 简易计算器 ---------- */
static char calc_display[32];
static int calc_pos;
static int calc_op;      /* 0=none, 1=+, 2=-, 3=*, 4=/ */
static int calc_val1;

static void calc_init(void) {
    calc_pos = 0; calc_op = 0; calc_val1 = 0;
    for (int i = 0; i < 32; i++) calc_display[i] = 0;
}

static void calc_input(char c) {
    if (c >= '0' && c <= '9') {
        if (calc_pos < 30) calc_display[calc_pos++] = c;
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        /* 解析当前显示的值 */
        int val = 0;
        for (int i = 0; i < calc_pos; i++)
            val = val * 10 + (calc_display[i] - '0');
        calc_val1 = val;
        calc_op = (c == '+') ? 1 : (c == '-') ? 2 : (c == '*') ? 3 : 4;
        calc_pos = 0;
    } else if (c == '=' || c == '\r' || c == '\n') {
        int val2 = 0;
        for (int i = 0; i < calc_pos; i++)
            val2 = val2 * 10 + (calc_display[i] - '0');
        int result = 0;
        if (calc_op == 1) result = calc_val1 + val2;
        else if (calc_op == 2) result = calc_val1 - val2;
        else if (calc_op == 3) result = calc_val1 * val2;
        else if (calc_op == 4 && val2 != 0) result = calc_val1 / val2;
        calc_pos = 0; calc_op = 0;
        /* 转字符串 */
        if (result == 0) { calc_display[0] = '0'; calc_pos = 1; }
        else {
            char tmp[16]; int i = 0;
            if (result < 0) { calc_display[calc_pos++] = '-'; result = -result; }
            while (result > 0) { tmp[i++] = '0' + (result % 10); result /= 10; }
            while (i > 0) calc_display[calc_pos++] = tmp[--i];
        }
    } else if (c == 'C' || c == 'c') {
        calc_init();
    }
    calc_display[calc_pos] = 0;
}

const char *calc_get_display(void) { return calc_display; }

/* ---------- 简易文本编辑器 ---------- */
#define EDIT_MAX 512
static char edit_buf[EDIT_MAX];
static int edit_len;

static void editor_init(void) {
    edit_len = 0;
    for (int i = 0; i < EDIT_MAX; i++) edit_buf[i] = 0;
    edit_buf[0] = 0;
}

static void editor_input(char c) {
    if (c == '\b' || c == 127) {
        if (edit_len > 0) edit_buf[--edit_len] = 0;
    } else if (c >= 32 && c <= 126 && edit_len < EDIT_MAX - 2) {
        edit_buf[edit_len++] = c;
        edit_buf[edit_len] = 0;
    } else if (c == '\r' || c == '\n') {
        if (edit_len < EDIT_MAX - 2) {
            edit_buf[edit_len++] = '\n';
            edit_buf[edit_len] = 0;
        }
    }
}

const char *editor_get_text(void) { return edit_buf; }

/* ---------- 文件浏览器 ---------- */
#define FILE_LIST_MAX 16

static char file_names[FILE_LIST_MAX][32];
static int file_count;

extern int ramfs_list_names(char names[16][32]);

static void browser_refresh(void) {
    file_count = ramfs_list_names(file_names);
}

const char *browser_get_file(int idx) {
    if (idx < 0 || idx >= file_count) return "";
    return file_names[idx];
}

int browser_get_count(void) { return file_count; }

/* ---------- 初始化所有应用 ---------- */
void apps_init(void) {
    calc_init();
    editor_init();
    browser_refresh();
    serial_puts("apps: initialized (calc, editor, browser)\r\n");
}

void apps_calc_input(char c) { calc_input(c); }
void apps_editor_input(char c) { editor_input(c); }
void apps_browser_refresh(void) { browser_refresh(); }
