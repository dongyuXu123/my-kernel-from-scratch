# F4: 桌面环境 (Day 34)

## 对照源码
- Desktop Metaphor (Xerox PARC, 1970s), Windows/Mac/Linux DE 概念

## 学习目标
1. 任务栏: 底部 32px 面板 + 启动器按钮 + 时钟
2. 系统时钟: PIT 100Hz → 每秒更新 HH:MM:SS
3. 应用启动器: desktop_add_launcher(name, callback) → 点击检测 → 调用回调

## 代码导读
- `arch/x86/desktop.c`: desktop_init, desktop_draw, desktop_tick, desktop_check_launcher
- 时钟: gui_tick_count 累积到 100 → 秒+1 → 分钟进位 → 小时进位

## 验证
```bash
cd course/day34 && make
qemu-system-x86_64 -kernel mykernel.elf -vga std -device i8042 \
  -display none -serial stdio -no-reboot -no-shutdown
```
