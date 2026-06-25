/* arch/x86/fb.c — Framebuffer 控制台 (Day 35) */
#include "kernel.h"

unsigned int *fb;      /* framebuffer 地址 */
int fb_width, fb_height, fb_pitch;
static int cursor_x, cursor_y;

/* 8x16 字体 (仅 ASCII 32-126) */
static unsigned char font[128][16] = {
    [65] = {0x00,0x38,0x44,0x44,0x7C,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* A */
    [66] = {0x00,0x78,0x44,0x44,0x78,0x44,0x44,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* B */
    [67] = {0x00,0x38,0x44,0x40,0x40,0x40,0x44,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* C */
    [72] = {0x00,0x44,0x44,0x44,0x7C,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* H */
    [76] = {0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* L */
    [79] = {0x00,0x38,0x44,0x44,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* O */
};

void fb_init(void) {
    /* 扫描 PCI 找 VGA (vendor=0x1234, device=0x1111) */
    extern unsigned int pci_read32(int,int,int,int);
    for (int dev = 0; dev < 32; dev++) {
        unsigned int v = pci_read32(0, dev, 0, 0);
        if ((v & 0xFFFF) == 0x1234 && ((v >> 16) & 0xFFFF) == 0x1111) {
            unsigned int bar0 = pci_read32(0, dev, 0, 0x10);
            fb = (unsigned int *)(unsigned long)(bar0 & 0xFFFFFFF0);
            extern void map_mmio(unsigned long);
            map_mmio((unsigned long)fb);
            /* 1024×768×4≈3MB, 需要映射第二个 2MB 页 */
            map_mmio((unsigned long)fb + 0x200000);
            fb_width = 1024; fb_height = 768; fb_pitch = 1024;
            cursor_x = cursor_y = 0;
            serial_puts("FB: VGA found at "); print_hex64((unsigned long)fb);
            serial_puts("\r\n");
            /* 清屏 (黑色) */
            for (int i = 0; i < fb_width * fb_height; i++) fb[i] = 0;
            return;
        }
    }
    serial_puts("FB: VGA not found\r\n");
}

void fb_putchar(char c) {
    if (!fb) return;
    if (c == '\n') { cursor_x = 0; cursor_y++; return; }
    if (c == '\r') { cursor_x = 0; return; }
    if (c < 32 || c > 126) c = '?';
    
    unsigned char *glyph = font[(unsigned char)c];
    for (int row = 0; row < 16; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            unsigned int color = (bits & (0x80 >> col)) ? 0x00AAAAAA : 0x00000000;
            int px = cursor_x * 8 + col;
            int py = cursor_y * 16 + row;
            if (px < fb_width && py < fb_height)
                fb[py * fb_pitch + px] = color;
        }
    }
    cursor_x++;
    if (cursor_x * 8 >= fb_width) { cursor_x = 0; cursor_y++; }
}

void fb_puts(const char *s) {
    while (*s) fb_putchar(*s++);
}

/* 在指定像素位置画字符串 (供窗口管理器使用) */
void fb_draw_text(int x, int y, const char *s, unsigned int color) {
    if (!fb || !s) return;
    while (*s) {
        unsigned char c = *s++;
        if (c < 32 || c > 126) c = '?';
        unsigned char *glyph = font[(unsigned char)c];
        for (int row = 0; row < 16; row++) {
            unsigned char bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col)) {
                    int px = x + col, py = y + row;
                    if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
                        fb[py * fb_pitch + px] = color;
                }
            }
        }
        x += 8;
    }
}
