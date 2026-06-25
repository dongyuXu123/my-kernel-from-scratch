#!/bin/bash
# 制作可启动 ISO 镜像
# 用法: ./scripts/make-iso.sh

KERNEL=mykernel/mykernel.elf
ISO=mykernel.iso
ISODIR=/tmp/iso_build

echo "Building ISO..."

# Ensure kernel is built
cd $(dirname $0)/..
make -C mykernel -j$(nproc) 2>/dev/null

# Setup ISO directory
rm -rf $ISODIR
mkdir -p $ISODIR/boot/grub

# Copy kernel
cp $KERNEL $ISODIR/boot/

# Create GRUB config
cat > $ISODIR/boot/grub/grub.cfg << GRUB
set timeout=3
menuentry "MyKernel" {
    multiboot /boot/mykernel.elf
    boot
}
GRUB

# Create ISO
grub-mkrescue -o $ISO $ISODIR 2>/dev/null

echo "Done: $ISO"
echo "Run: qemu-system-x86_64 -cdrom $ISO"
