#!/bin/bash
# BusyBox 交叉编译 — MyKernel 专用
# 用法: ./scripts/build-busybox.sh
# 输出: busybox 静态二进制 → 复制到 mykernel/initfs/
set -e
MUSL_VER=1.2.5
BUSYBOX_VER=1.36.1
CROSS_PREFIX=$HOME/mykernel-cross

echo "=== Downloading musl-${MUSL_VER} ==="
wget -q https://musl.libc.org/releases/musl-${MUSL_VER}.tar.gz
tar xzf musl-${MUSL_VER}.tar.gz
cd musl-${MUSL_VER}
./configure --prefix=${CROSS_PREFIX} --disable-shared CFLAGS="-static -fno-pie"
make -j$(nproc) && make install
cd ..

echo "=== Downloading busybox-${BUSYBOX_VER} ==="
wget -q https://busybox.net/downloads/busybox-${BUSYBOX_VER}.tar.bz2
tar xjf busybox-${BUSYBOX_VER}.tar.bz2
cd busybox-${BUSYBOX_VER}
make defconfig
sed -i 's/.*CONFIG_STATIC.*/CONFIG_STATIC=y/' .config
make -j$(nproc)
echo "Done: busybox binary at $(pwd)/busybox"
