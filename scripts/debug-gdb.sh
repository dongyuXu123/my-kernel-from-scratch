#!/bin/bash
#
# debug-gdb.sh - gdb 远程调试 mykernel
#
# 前置: 另一终端先运行 ./scripts/run-qemu.sh -d (QEMU 会挂起等待)
# 本脚本连接 QEMU 的 gdb stub(localhost:1234)
#
# 连上后在 _start 下断点,然后 continue,让 QEMU 跑到入口。

set -e
cd "$(dirname "$0")/.."   # 切到课程根目录

KERNEL=mykernel/mykernel.elf
GDB=gdb

	# 注意: 内核是 ELF32 但运行在 64 位模式,gdb 需要 set arch 才能正确解码
	$GDB "$KERNEL" \
	    -ex "set architecture i386:x86-64" \
	    -ex "target remote localhost:1234" \
	    -ex "break _start" \
	    -ex "break start64" \
	    -ex "continue"
