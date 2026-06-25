CC := gcc
LD := ld
CFLAGS := -m32 -ffreestanding -nostdlib -Wall -fno-pie -fno-stack-protector -I include
LDFLAGS := -nostdlib -T linker.ld

# Auto-discover source files
ASM_SRCS := $(shell find . -name "*.S" -not -path "./initfs/*" 2>/dev/null)
C_SRCS   := $(shell find . -name "*.c" 2>/dev/null)
ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS   := $(C_SRCS:.c=.o)
OBJS     := $(ASM_OBJS) $(C_OBJS)
KERNEL   := mykernel.elf

.PHONY: all clean run

all: $(KERNEL)

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

run: $(KERNEL)
	timeout 3 qemu-system-x86_64 -kernel $(KERNEL) -display none -serial file:/tmp/mykernel-out.log -no-reboot -no-shutdown 2>&1 || true
	cat /tmp/mykernel-out.log

clean:
	rm -f $(OBJS) $(KERNEL)
