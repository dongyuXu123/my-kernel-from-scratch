#!/bin/bash
#
# run-qemu.sh - 用 QEMU 加载 mykernel
#
# 用法:
#   ./scripts/run-qemu.sh           交互模式(串口→stdio, 可用 shell)
#   ./scripts/run-qemu.sh -d        调试模式(QEMU 挂起等待 gdb)
#   ./scripts/run-qemu.sh -q        快速验证(3s 超时, 串口→文件)

set -e
cd "$(dirname "$0")/.."

KERNEL=mykernel/mykernel.elf
QEMU=qemu-system-x86_64
SERIAL_LOG=/tmp/mykernel-serial.log

case "${1:-}" in
    -d)
        echo "[*] 调试模式: QEMU 挂起,等待 gdb localhost:1234"
        $QEMU -kernel "$KERNEL" -serial stdio -no-reboot -no-shutdown -s -S
        ;;
    -q)
        timeout 3 $QEMU -kernel "$KERNEL" -display none \
            -serial file:"$SERIAL_LOG" -no-reboot -no-shutdown 2>&1 || true
        echo "[*] --- 串口输出 ---"
        cat "$SERIAL_LOG" 2>/dev/null || echo "(无输出)"
        ;;
    *)
        echo "[*] 交互模式: Ctrl-A X 退出"
        echo "[*] 提示: 需在终端中运行以使用 shell"
        $QEMU -kernel "$KERNEL" -nographic -no-reboot -no-shutdown
        ;;
esac
