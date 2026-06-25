# F5: GUI 应用程序 (Day 35)

## 对照源码
- MVC Pattern, Windows Notepad/Calculator 概念

## 学习目标
1. 简易计算器: 字符解析 + 四则运算 (状态机: NUM→OP→=→C)
2. 文本编辑器: 字符缓冲区 edit_buf[512], 退格+换行支持
3. 文件浏览器: ramfs_list_names() → 读取文件列表

## 代码导读
- `arch/x86/apps.c`: apps_init, apps_calc_input, apps_editor_input, apps_browser_refresh
- 计算器: 输入 '1' '2' '+' '3' '=' → 12+3=15

## 验证
```bash
cd course/day35 && make
qemu-system-x86_64 -kernel mykernel.elf -vga std -device i8042 \
  -display none -serial stdio -no-reboot -no-shutdown
```
