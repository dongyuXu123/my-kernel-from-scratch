# F3: GUI 控件 (Day 33)

## 对照源码
> **状态**: QEMU验证通过 ✓
- GTK/Qt Widget 模型, Windows GDI 概念

## 学习目标
> **状态**: QEMU验证通过 ✓
1. 控件类型: Button (3D边框+点击回调), Label (纯文字), Input (键盘输入+光标)
2. 3D 边框效果 (凸起: 左上亮色+右下暗色; 凹陷: 相反)
3. 事件分发: 鼠标点击 → widget_handle_click() → 命中检测 → on_click(id)
4. 键盘焦点: PS/2 扫描码→ASCII→widget_handle_key()→焦点文本框

## 代码导读
> **状态**: QEMU验证通过 ✓
- `arch/x86/widget.c`: widget_create_button/label/input, widget_handle_click, widget_handle_key
- 控件列表: 全局数组 widgets[64], 逆序遍历实现 Z-order

## 验证
> **状态**: QEMU验证通过 ✓
```bash
cd course/day33 && make
qemu-system-x86_64 -kernel mykernel.elf -vga std -device i8042 \
  -display none -serial stdio -no-reboot -no-shutdown
```
