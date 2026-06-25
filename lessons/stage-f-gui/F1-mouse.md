# F1: PS/2 鼠标驱动 (Day 31)

## 对照源码
> **状态**: QEMU验证通过 ✓
- `reference/linux-7.1/drivers/input/mouse/psmouse-base.c` (Linux PS/2 鼠标)
- OSDev Wiki: PS/2 Mouse

## 学习目标
> **状态**: QEMU验证通过 ✓
1. 理解 PS/2 控制器双通道架构 (端口 0x60=数据, 0x64=状态/命令)
2. 掌握鼠标初始化流程 (启用辅助设备 → 设置采样率 → 启用数据报告)
3. 解析 3 字节鼠标数据包 (按钮 + X/Y 位移 + 符号位)
4. 在 Framebuffer 上绘制像素级光标

## 背景知识
> **状态**: QEMU验证通过 ✓

### PS/2 控制器
IBM PS/2 控制器管理两个设备：键盘 (端口 1) 和鼠标 (端口 2)。通信通过两个 I/O 端口：

| 端口 | 方向 | 功能 |
|------|------|------|
| 0x60 | 读/写 | 数据寄存器 |
| 0x64 | 读 | 状态寄存器 (bit0=输出满, bit1=输入满, bit5=鼠标数据) |
| 0x64 | 写 | 命令寄存器 |

### 鼠标初始化序列
```
1. 写 0xA8 → 0x64  (启用辅助设备/鼠标端口)
2. 写 0x20 → 0x64, 读 0x60 (获取当前配置字节)
3. 写 0x60 → 0x64, 写 (cfg|2) → 0x60  (启用鼠标中断+时钟)
4. 写 0xD4 → 0x64, 写 0xF4 → 0x60  (启用鼠标数据报告)
5. 读 0x60 等待 ACK (0xFA)
```

### 3 字节数据包
```
Byte1: Yov Xov Ysign Xsign 1 Middle Right Left
Byte2: X 位移 (8位有符号 + 第9位在 Byte1.Xsign)
Byte3: Y 位移 (8位有符号 + 第9位在 Byte1.Ysign, 需取反)
```

### 光标绘制
在 Framebuffer 上直接操作像素数组 `fb[y * pitch + x] = color`。使用 8×12 简单箭头形状，移动时擦除旧位置(黑色填充)再画新位置。

## 代码导读
> **状态**: QEMU验证通过 ✓

### arch/x86/mouse.c
- `mouse_wait(type)`: 轮询 PS/2 状态寄存器，等待数据就绪或空闲
- `mouse_init()`: 完整的鼠标初始化序列
- `mouse_poll()`: 读取 3 字节并解析为 x/y/buttons
- `mouse_draw()`: 在 fb 上绘制 8×12 白色箭头光标

### 关键设计决策
- **轮询而非中断**: 简化实现，主循环中主动调用 `mouse_poll()`
- **直接 Framebuffer 访问**: 绕过窗口系统，直接在物理 fb 上画像素
- **边界裁剪**: 鼠标坐标限制在 0..fb_width-1, 0..fb_height-1

## 验证
> **状态**: QEMU验证通过 ✓
```bash
cd course/day31 && make
qemu-system-x86_64 -kernel mykernel.elf -vga std -device i8042 \
  -display none -serial stdio -no-reboot -no-shutdown
# 预期: mouse: initialized, 移动鼠标可见光标
```
