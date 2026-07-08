# KERNEL — Makefile
#
# Build the disk image as: [ boot sector | C kernel ], where the boot sector
# loads the kernel off disk and switches to 32-bit protected mode before
# handing control to kmain.
#
# Targets:
#   make            build build/os-image.bin
#   make run        boot the image in QEMU (opens a window)
#   make debug      like run, but wait for GDB on :1234
#   make screenshot boot headless, dump the screen to build/screen.png
#   make clean      remove build/

BUILD  := build
BOOT   := $(BUILD)/boot.bin
KERNEL := $(BUILD)/kernel.bin
IMAGE  := $(BUILD)/os-image.bin

CC      := i686-elf-gcc
LD      := i686-elf-ld
OBJCOPY := i686-elf-objcopy
QEMU    := qemu-system-i386
QEMUFLAGS := -m 32M

CFLAGS := -ffreestanding -std=gnu11 -O2 -Wall -Wextra \
          -fno-stack-protector -fno-pic -Ikernel

C_SOURCES   := $(wildcard kernel/*.c)
ASM_SOURCES := $(filter-out kernel/entry.asm,$(wildcard kernel/*.asm))
C_OBJ       := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SOURCES))
ASM_OBJ     := $(patsubst kernel/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))
# entry.o must come first so _start lands at 0x10000.
KOBJ        := $(BUILD)/entry.o $(ASM_OBJ) $(C_OBJ)

.PHONY: all run debug screenshot web clean

all: $(IMAGE)

# Stage the built image into web/ so the v86 in-browser boot serves the latest.
web: $(IMAGE)
	cp $(IMAGE) web/os-image.bin
	@echo "staged web/os-image.bin — serve web/ (see .claude/launch.json)"

$(BUILD):
	mkdir -p $(BUILD)

# --- boot sector: assemble straight to a flat 512-byte binary ---
$(BOOT): boot/boot.asm | $(BUILD)
	nasm -f bin $< -o $@

# --- assembly sources (entry stub + interrupt stubs) -> ELF objects ---
$(BUILD)/%.o: kernel/%.asm | $(BUILD)
	nasm -f elf32 $< -o $@

# --- the ring-3 shell: keep its constants un-merged so ALL of its read-only
#     data stays in shell.o (and thus in the linker's .user_text section that
#     paging marks user-accessible). Otherwise string-merging scatters its
#     literals into kernel .rodata and ring 3 faults reading them. ---
$(BUILD)/shell.o: kernel/shell.c | $(BUILD)
	$(CC) $(CFLAGS) -fno-merge-constants -c $< -o $@

# --- C sources -> ELF objects ---
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# --- link the kernel (entry.o first so _start is at 0x10000), flatten it ---
$(KERNEL): $(KOBJ) linker.ld
	$(LD) -T linker.ld -o $(BUILD)/kernel.elf $(KOBJ)
	$(OBJCOPY) -O binary $(BUILD)/kernel.elf $@

# --- final image: boot sector at sector 0, kernel from sector 1 ---
# Pad to 1 MiB (2048 sectors). A tiny disk yields 0 cylinders under standard
# 16x63 CHS geometry, which some BIOSes (notably v86's SeaBIOS) reject as
# unbootable — 1 MiB gives valid geometry while staying trivially small.
$(IMAGE): $(BOOT) $(KERNEL)
	dd if=/dev/zero of=$@ bs=512 count=2048 2>/dev/null
	dd if=$(BOOT)   of=$@ conv=notrunc 2>/dev/null
	dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

run: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -drive format=raw,file=$(IMAGE)

debug: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -drive format=raw,file=$(IMAGE) -s -S

# Headless verification: boot with no display, wait for POST + our code to run,
# dump the VGA framebuffer (QEMU writes PPM), convert to PNG.
screenshot: $(IMAGE)
	( echo "screendump $(BUILD)/screen.ppm"; sleep 4; \
	  echo "screendump $(BUILD)/screen.ppm"; sleep 1; echo "quit" ) | \
	$(QEMU) $(QEMUFLAGS) -drive format=raw,file=$(IMAGE) -display none -monitor stdio >/dev/null 2>&1
	sips -s format png $(BUILD)/screen.ppm --out $(BUILD)/screen.png >/dev/null
	@echo "wrote $(BUILD)/screen.png"

clean:
	rm -rf $(BUILD)
