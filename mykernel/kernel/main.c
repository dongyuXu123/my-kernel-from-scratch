/*
 * kernel/main.c — 内核主初始化 (含 GUI 支持)
 *
 * 对照: reference/linux-7.1/init/main.c (start_kernel)
 */
#include "kernel.h"
#include "mm.h"
#include "sched.h"
#include "module.h"

extern void gdt_init(void), idt_init(void), tss_init(void);
extern void ramfs_init(void);
extern int  ramfs_create(const char *name);
extern int  ramfs_write(int fd, const char *buf, int len);
extern void enter_user_mode_asm(void *);
extern void fb_init(void), fb_puts(const char *);

/* ===== GUI 全局状态 ===== */
int gui_mode = 0;   /* 0=console, 1=GUI */

/* ===== GUI 主循环 ===== */
static void gui_loop(void)
{
    extern int mouse_get_x(void), mouse_get_y(void), mouse_get_buttons(void);
    extern void mouse_poll(void), mouse_draw(void);
    extern void wm_handle_move(int,int), wm_handle_release(void);
    extern void wm_redraw_all(void);
    extern int  wm_create_window(int,int,int,int,const char *);
    extern void widget_draw_all(void);
    extern int  widget_handle_click(int,int);
    extern void widget_handle_key(char);
    extern void desktop_draw(void), desktop_tick(void);
    extern void *desktop_check_launcher(int,int);
    extern unsigned int *fb;
    extern int fb_pitch, fb_width, fb_height;
    extern unsigned long timer_ticks;

    int last_btn = 0;
    int last_mx = 0, last_my = 0;
    unsigned long last_tick = 0;
    
    while (1) {
        mouse_poll();
        int mx = mouse_get_x(), my = mouse_get_y(), btn = mouse_get_buttons();
        
        /* 时钟: 检测 timer_ticks 变化(100Hz → 10 ticks = 100ms 精度) */
        unsigned long now = timer_ticks;
        if (now - last_tick >= 10) {
            desktop_tick();
            last_tick = now;
        }
        
        /* 鼠标移动 — 拖拽窗口 */
        if (mx != last_mx || my != last_my) {
            /* 简单擦除旧光标区域 */
            if (fb) {
                for (int y = 0; y < 12; y++)
                    for (int x = 0; x < 8; x++) {
                        int px = last_mx + x, py = last_my + y;
                        if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
                            fb[py * fb_pitch + px] = 0;
                    }
            }
            wm_handle_move(mx, my);
            wm_redraw_all();
            widget_draw_all();
            desktop_draw();
            mouse_draw();
            last_mx = mx; last_my = my;
        }
        
        /* 鼠标按下 — 处理点击 */
        if (btn && !last_btn) {
            /* 先检查桌面启动器 */
            void (*launch)(void) = desktop_check_launcher(mx, my);
            if (launch) { launch(); }
            /* 再检查窗口 */
            extern void wm_handle_click(int,int,int);
            wm_handle_click(mx, my, btn);
            /* 再检查控件 */
            widget_handle_click(mx, my);
        }
        
        /* 鼠标释放 */
        if (!btn && last_btn) {
            wm_handle_release();
        }
        
        /* 键盘处理 */
        {
            unsigned char kbd_stat = inb(0x64);
            if (kbd_stat & 1) {
                unsigned char sc = inb(0x60);
                if (sc < 0x80) {
                    char c = 0;
                    static const char kbd_map[128] = {
                        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,
                        0,'q','w','e','r','t','y','u','i','o','p','[',']',0,
                        0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,
                        0,'\\','z','x','c','v','b','n','m',',','.','/',0,
                        0,0,0,' ',0
                    };
                    c = kbd_map[sc];
                    if (c) {
                        widget_handle_key(c);
                        /* 同时路由到应用层(计算器/编辑器) */
                        extern void apps_calc_input(char);
                        extern void apps_editor_input(char);
                        apps_calc_input(c);
                        apps_editor_input(c);
                    }
                }
            }
        }
        
        last_btn = btn;
        __asm__ volatile ("hlt");
    }
}

void start_kernel(void)
{
    serial_init();
    serial_puts("Hello from my kernel\r\n");

    gdt_init(); idt_init();
    pmm_init(); setup_pagetables(); buddy_init(); slab_init();
    extern void cow_setup(void);
    cow_setup();  /* Day 46: COW 初始化 */
    sched_init(); tss_init();

    /* 初始化定时器(PIC + PIT) */
    extern void pic_remap(void);
    extern void pic_enable_irq(int irq);
    extern void pit_init(unsigned int hz);
    pic_remap();
    pit_init(100);       /* 100Hz */
    pic_enable_irq(0);   /* 开 IRQ0 */

    /* 内核符号表 */
    register_kernel_symbol("serial_puts", serial_puts);
    register_kernel_symbol("print_hex64", print_hex64);

    /* PCI 枚举 */
    extern void pci_scan(void);
    pci_scan();

    /* e1000 初始化 */
    extern void e1000_init(void);
    e1000_init();

    /* TCP 服务器模式 (监听 port 80) */
    extern void tcp_listen(void);
    tcp_listen();

    /* DHCP 客户端 */
    extern void dhcp_init(void);
    dhcp_init();
    extern void dhcp_discover(void);
    dhcp_discover();

    /* SMP 多核 */
    extern void smp_init(void);
    smp_init();
    extern void apic_init(void);
    apic_init();

    /* ramfs */
    ramfs_init();
    ramfs_create("hello.txt");
    ramfs_write(0, "Hello from ramfs!", 17);
    ramfs_create("readme.txt");
    ramfs_write(1, "Welcome!\r\n", 10);

    /* 预装程序 */
    extern unsigned char hello_elf_start[], hello_elf_end[];
    int hl = (int)(hello_elf_end - hello_elf_start);
    ramfs_create("hello");
    ramfs_write(2, (char *)hello_elf_start, hl);

    /* 预装模块 */
    extern unsigned char mod_elf_start[], mod_elf_end[];
    int ml = (int)(mod_elf_end - mod_elf_start);
    ramfs_create("hello_mod.o");
    ramfs_write(3, (char *)mod_elf_start, ml);

    /* ---- GUI 初始化 ---- */
    extern unsigned int *fb;
    extern int wm_create_window(int,int,int,int,const char*);
    int console_boot = 0;  /* 1=busybox, 0=GUI */

    /* Framebuffer */
    fb_init();
    if (!console_boot && fb) {
        gui_mode = 1;
        serial_puts("Starting GUI mode...\r\n");
        
        /* 鼠标 */
        extern void mouse_init(void);
        mouse_init();
        
        /* 窗口管理器 */
        extern void wm_init(void);
        wm_init();
        
        /* 桌面 */
        extern void desktop_init(void);
        extern void desktop_add_launcher(const char*,void(*)(void));
        desktop_init();
        
        void launch_calc(void)  { serial_puts("Launcher: Calculator\r\n"); }
        void launch_edit(void)  { serial_puts("Launcher: Editor\r\n"); }
        void launch_files(void) { serial_puts("Launcher: File Browser\r\n"); }
        desktop_add_launcher("Calc", launch_calc);
        desktop_add_launcher("Edit", launch_edit);
        desktop_add_launcher("Files", launch_files);
        
        /* 控件 */
        /* (widgets 不需要单独初始化) */
        
        /* 应用 */
        extern void apps_init(void);
        apps_init();
        
        /* 创建演示窗口 */
        wm_create_window(50, 50, 320, 200, "Demo Window");
        wm_create_window(150, 100, 400, 300, "Second Window");
        
        /* 创建控件 */
        extern int widget_create_button(int,int,int,const char*,unsigned int,unsigned int,void(*)(int));
        extern int widget_create_label(int,int,const char*,unsigned int);
        
        widget_create_label(60, 80, "Welcome to ZCode GUI!", 0x00FFFFFF);
        widget_create_button(60, 110, 120, "Click Me", 0x00404080, 0x00FFFFFF, 0);
        
        /* 进入 GUI 主循环 */
        gui_loop();
        /* 永远不会返回 */
    }

    /* 无 framebuffer 时回退到 console 模式 */
    serial_puts("No framebuffer, starting console mode...\r\n");

    /* 尝试加载 BusyBox (如果存在) */
    extern unsigned char busybox_elf_start[], busybox_elf_end[];
    extern unsigned long elf_load_mem(void *, unsigned int);
    unsigned long bbsize = (unsigned long)(busybox_elf_end - busybox_elf_start);
    if (bbsize > 1024) {
        serial_puts("Loading BusyBox (");
        print_hex64(bbsize);
        serial_puts(" bytes)...\r\n");
        unsigned long entry = elf_load_mem(busybox_elf_start, (unsigned int)bbsize);
        if (entry) {
            extern unsigned long sched_ready;
            sched_ready = 1;
            serial_puts("BusyBox entry=");
            print_hex64(entry);
            serial_puts("\r\n");
            enter_user_mode_asm((void *)entry);
        }
    }

    /* 加载 /init */
    extern unsigned char init_elf_start[], init_elf_end[];
    unsigned long entry = elf_load_mem(init_elf_start,
                        (unsigned int)(init_elf_end - init_elf_start));
    if (entry) {
        extern unsigned long sched_ready;
        sched_ready = 1;
        enter_user_mode_asm((void *)entry);
    }
    while (1) { __asm__ volatile ("hlt"); }
}
