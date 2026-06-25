# F2: 窗口管理器 (Day 32)

## 对照源码
> **状态**: QEMU验证通过 ✓
- tinywm (github.com/mackstann/tinywm) — X11 Window Manager 概念

## 学习目标
> **状态**: QEMU验证通过 ✓
1. 窗口抽象 (struct window: x, y, w, h, title, visible, bg_color)
2. fill_rect 矩形填充 + 窗口绘制 (背景→标题栏→边框→文字)
3. 标题栏拖拽移动 (drag_off 偏移量计算)
4. Z-order 和窗口重绘 (逆序绘制 = 最顶层最后画)

## 代码导读
> **状态**: QEMU验证通过 ✓
- `arch/x86/wm.c`: wm_create_window, wm_handle_click, wm_handle_move, wm_redraw_all
- 拖拽原理: 记录 drag_off_x/y = mx - win->x, 移动时 win->x = mx - drag_off_x

## 验证
> **状态**: QEMU验证通过 ✓
```bash
cd course/day32 && make
qemu-system-x86_64 -kernel mykernel.elf -vga std -device i8042 \
  -display none -serial stdio -no-reboot -no-shutdown
```
