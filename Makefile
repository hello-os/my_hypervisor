# Makefile for my_hypervisor
# Sibling to xvisor-0.3.2
# Designed for ARMv8 AArch64 baremetal / EL2 Hypervisor

CROSS_COMPILE ?= aarch64-linux-gnu-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS  = -Wall -Werror -O2 -g -nostdlib -nostartfiles -ffreestanding -I.
ASFLAGS = -Wall -g -nostdlib -ffreestanding -I.

OBJS = start.o main.o mm.o irq.o vcpu.o vcpu_asm.o vmm_heap.o dt_parser.o vmm_shell.o stage2_mmu.o

all: my_hypervisor.elf my_hypervisor.bin

my_hypervisor.elf: $(OBJS) linker.ld
	$(LD) -T linker.ld -o my_hypervisor.elf $(OBJS)

my_hypervisor.bin: my_hypervisor.elf
	$(OBJCOPY) -O binary my_hypervisor.elf my_hypervisor.bin

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.elf *.bin

.PHONY: all clean
