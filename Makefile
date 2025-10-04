# ramOS Makefile
# Builds a Multiboot v1 compliant kernel with initrd for i686 architecture

# Toolchain
CC = i686-elf-gcc
LD = i686-elf-ld
AS = nasm
AR = i686-elf-ar

# Version from VERSION file
VERSION := $(shell cat VERSION)

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub
INITRD_ROOT = initrd_root

# Output files
KERNEL_ELF = $(BOOT_DIR)/kernel.elf
INITRD_CPIO = $(BOOT_DIR)/initrd.cpio
ISO_FILE = $(ISO_DIR)/ramOS.iso

# Source files
BOOT_ASM = $(SRC_DIR)/boot/boot.asm
BOOT_OBJ = $(BUILD_DIR)/boot.o

KERNEL_SOURCES = $(wildcard $(SRC_DIR)/kernel/*.c)
KERNEL_OBJECTS = $(patsubst $(SRC_DIR)/kernel/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SOURCES))

LINKER_SCRIPT = $(SRC_DIR)/linker.ld

# Compiler flags
CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -Wall -Wextra -O2 -DKERNEL_VERSION=\"$(VERSION)\" \
         -I$(SRC_DIR)/kernel

LDFLAGS = -m elf_i386 -T $(LINKER_SCRIPT) -nostdlib

ASFLAGS = -f elf32

# Default target
.PHONY: all
all: toolchain-check kernel initrd iso

# Check toolchain availability
.PHONY: toolchain-check
toolchain-check:
	@chmod +x scripts/toolchain_check.sh
	@./scripts/toolchain_check.sh

# Build kernel
.PHONY: kernel
kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(BOOT_OBJ) $(KERNEL_OBJECTS) $(LINKER_SCRIPT) | $(BOOT_DIR)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJECTS)
	@echo "Kernel built: $@"

# Compile boot assembly
$(BOOT_OBJ): $(BOOT_ASM) | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(AS) $(ASFLAGS) $< -o $@

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/kernel/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build initrd
.PHONY: initrd
initrd: $(INITRD_CPIO)

$(INITRD_CPIO): $(wildcard $(INITRD_ROOT)/*) | $(BOOT_DIR)
	@echo "Creating initrd..."
	@chmod +x scripts/make_initrd.sh
	@./scripts/make_initrd.sh

# Build ISO
.PHONY: iso
iso: $(ISO_FILE)

$(ISO_FILE): $(KERNEL_ELF) $(INITRD_CPIO) $(GRUB_DIR)/grub.cfg
	@echo "Creating ISO image..."
	@if [ ! -f "$(GRUB_DIR)/grub.cfg" ]; then \
		echo "Error: grub.cfg not found at $(GRUB_DIR)/grub.cfg"; \
		exit 1; \
	fi
	grub-mkrescue -o $@ $(ISO_DIR)
	@echo "ISO created: $@"

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BOOT_DIR):
	mkdir -p $(BOOT_DIR)

$(GRUB_DIR):
	mkdir -p $(GRUB_DIR)

# Run in VirtualBox
.PHONY: run-vbox
run-vbox: $(ISO_FILE)
	@chmod +x scripts/run_virtualbox.sh
	@./scripts/run_virtualbox.sh $(ISO_FILE)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(KERNEL_ELF) $(INITRD_CPIO) $(ISO_FILE)
	@echo "Clean complete"

# Deep clean (including ISO directory)
.PHONY: distclean
distclean: clean
	rm -rf $(ISO_DIR)
	@echo "Deep clean complete"

# Help
.PHONY: help
help:
	@echo "ramOS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build everything (default)"
	@echo "  toolchain-check- Verify required tools are available"
	@echo "  kernel         - Build kernel.elf"
	@echo "  initrd         - Create initrd.cpio"
	@echo "  iso            - Create bootable ISO"
	@echo "  run-vbox       - Run in VirtualBox"
	@echo "  clean          - Remove build artifacts"
	@echo "  distclean      - Remove all generated files"
	@echo "  help           - Show this help"
	@echo ""
	@echo "Version: $(VERSION)"