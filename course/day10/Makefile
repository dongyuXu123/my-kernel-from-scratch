CC := gcc
LD := ld
CFLAGS := -m64 -mcmodel=large -mno-red-zone -ffreestanding -nostdlib -Wall -Wextra -fno-pie -fno-stack-protector -I include
LDFLAGS := -nostdlib -T linker.ld

ASM_SRCS := $(shell find . -name "*.S" -not -path "./initfs/*" 2>/dev/null)
C_SRCS   := $(shell find . -name "*.c" 2>/dev/null)
ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS   := $(C_SRCS:.c=.o)
OBJS     := $(ASM_OBJS) $(C_OBJS)
KERNEL   := mykernel.elf

.PHONY: all clean

all: $(KERNEL)

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(KERNEL)
