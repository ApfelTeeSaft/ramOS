# ramOS Makefile - Complete build system with apps and drivers
# Builds kernel, system API library, applications, and creates bootable ISO

# Toolchain
CC = i686-elf-gcc
LD = i686-elf-ld
AS = nasm
AR = i686-elf-ar

# Version
VERSION := $(shell cat VERSION)

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub
INITRD_ROOT = initrd_root
APPS_DIR = $(SRC_DIR)/apps
API_DIR = $(SRC_DIR)/api

# Output files
KERNEL_ELF = $(BOOT_DIR)/kernel.elf
INITRD_CPIO = $(BOOT_DIR)/initrd.cpio
ISO_FILE = $(ISO_DIR)/ramOS.iso
LIBSYS = $(BUILD_DIR)/libsys.a

# Kernel sources
BOOT_ASM = $(SRC_DIR)/boot/boot.asm
BOOT_OBJ = $(BUILD_DIR)/boot.o

KERNEL_SOURCES = $(wildcard $(SRC_DIR)/kernel/core/*.c) \
                 $(wildcard $(SRC_DIR)/kernel/mm/*.c) \
                 $(wildcard $(SRC_DIR)/kernel/drivers/*.c) \
                 $(wildcard $(SRC_DIR)/kernel/fs/*.c) \
                 $(wildcard $(SRC_DIR)/kernel/proc/*.c) \
                 $(wildcard $(SRC_DIR)/kernel/api/*.c) \
                 $(SRC_DIR)/kernel/shell.c

KERNEL_OBJECTS = $(patsubst $(SRC_DIR)/kernel/%.c,$(BUILD_DIR)/kernel_%.o,$(KERNEL_SOURCES))

# API library sources
API_SOURCES = $(API_DIR)/libsys.c
API_OBJECTS = $(patsubst $(API_DIR)/%.c,$(BUILD_DIR)/api_%.o,$(API_SOURCES))

# Application sources
APP_NAMES = calculator editor filemanager installer shell
APP_BINARIES = $(foreach app,$(APP_NAMES),$(INITRD_ROOT)/bin/$(app).elf)

# Linker scripts
KERNEL_LINKER = $(SRC_DIR)/linker.ld
APP_LINKER = $(APPS_DIR)/common/app.ld

# Compiler flags
KERNEL_CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
                -Wall -Wextra -O2 -DKERNEL_VERSION=\"$(VERSION)\" \
                -I$(SRC_DIR)/kernel

API_CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector \
             -Wall -Wextra -O2 -I$(API_DIR)

APP_CFLAGS = -m32 -ffreestanding -fno-builtin -nostdlib -fno-stack-protector \
             -Wall -Wextra -O2 -I$(API_DIR)

LDFLAGS = -m elf_i386 -nostdlib
KERNEL_LDFLAGS = $(LDFLAGS) -T $(KERNEL_LINKER)
APP_LDFLAGS = $(LDFLAGS) -T $(APP_LINKER)

ASFLAGS = -f elf32

# Default target
.PHONY: all
all: toolchain-check kernel libsys apps initrd iso

# Check toolchain
.PHONY: toolchain-check
toolchain-check:
	@chmod +x scripts/toolchain_check.sh
	@./scripts/toolchain_check.sh

# Build kernel
.PHONY: kernel
kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(BOOT_OBJ) $(KERNEL_OBJECTS) $(KERNEL_LINKER) | $(BOOT_DIR)
	@echo "Linking kernel..."
	$(LD) $(KERNEL_LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJECTS)
	@echo "Kernel built: $@"

# Compile boot assembly
$(BOOT_OBJ): $(BOOT_ASM) | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(AS) $(ASFLAGS) $< -o $@

# Compile kernel C sources
$(BUILD_DIR)/kernel_%.o: $(SRC_DIR)/kernel/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

# Build system API library
.PHONY: libsys
libsys: $(LIBSYS)

$(LIBSYS): $(API_OBJECTS) | $(BUILD_DIR)
	@echo "Creating libsys.a..."
	$(AR) rcs $@ $(API_OBJECTS)
	@echo "Library built: $@"

$(BUILD_DIR)/api_%.o: $(API_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(API_CFLAGS) -c $< -o $@

# Build applications
.PHONY: apps
apps: $(APP_BINARIES)

# Calculator
$(INITRD_ROOT)/bin/calculator.elf: $(APPS_DIR)/calculator/calculator.c $(LIBSYS) $(APP_LINKER) | $(INITRD_ROOT)/bin
	@echo "Building calculator..."
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/calculator/calculator.c -o $(BUILD_DIR)/calculator.o
	$(LD) $(APP_LDFLAGS) -o $@ $(BUILD_DIR)/calculator.o $(LIBSYS)
	@./scripts/update_manifest.sh $(APPS_DIR)/calculator/manifest.yaml

# Editor
$(INITRD_ROOT)/bin/editor.elf: $(APPS_DIR)/editor/editor.c $(LIBSYS) $(APP_LINKER) | $(INITRD_ROOT)/bin
	@echo "Building editor..."
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/editor/editor.c -o $(BUILD_DIR)/editor.o
	$(LD) $(APP_LDFLAGS) -o $@ $(BUILD_DIR)/editor.o $(LIBSYS)
	@./scripts/update_manifest.sh $(APPS_DIR)/editor/manifest.yaml

# File Manager
$(INITRD_ROOT)/bin/filemanager.elf: $(APPS_DIR)/filemanager/filemanager.c $(LIBSYS) $(APP_LINKER) | $(INITRD_ROOT)/bin
	@echo "Building filemanager..."
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/filemanager/filemanager.c -o $(BUILD_DIR)/filemanager.o
	$(LD) $(APP_LDFLAGS) -o $@ $(BUILD_DIR)/filemanager.o $(LIBSYS)
	@./scripts/update_manifest.sh $(APPS_DIR)/filemanager/manifest.yaml

# Installer (multiple source files)
$(INITRD_ROOT)/bin/installer.elf: $(wildcard $(APPS_DIR)/installer/*.c) $(LIBSYS) $(APP_LINKER) | $(INITRD_ROOT)/bin
	@echo "Building installer..."
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/installer/installer.c -o $(BUILD_DIR)/installer.o
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/installer/partition.c -o $(BUILD_DIR)/partition.o
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/installer/format.c -o $(BUILD_DIR)/format.o
	$(LD) $(APP_LDFLAGS) -o $@ $(BUILD_DIR)/installer.o $(BUILD_DIR)/partition.o $(BUILD_DIR)/format.o $(LIBSYS)
	@./scripts/update_manifest.sh $(APPS_DIR)/installer/manifest.yaml

# Shell app
$(INITRD_ROOT)/bin/shell.elf: $(APPS_DIR)/shell/shell_app.c $(LIBSYS) $(APP_LINKER) | $(INITRD_ROOT)/bin
	@echo "Building shell..."
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/shell/shell_app.c -o $(BUILD_DIR)/shell_app.o
	$(LD) $(APP_LDFLAGS) -o $@ $(BUILD_DIR)/shell_app.o $(LIBSYS)
	@./scripts/update_manifest.sh $(APPS_DIR)/shell/manifest.yaml

# Build initrd
.PHONY: initrd
initrd: $(INITRD_CPIO)

$(INITRD_CPIO): $(APP_BINARIES) $(wildcard $(INITRD_ROOT)/*) | $(BOOT_DIR)
	@echo "Creating initrd..."
	@chmod +x scripts/make_initrd.sh
	@./scripts/make_initrd.sh

# Build ISO
.PHONY: iso
iso: $(ISO_FILE)

$(ISO_FILE): $(KERNEL_ELF) $(INITRD_CPIO) $(GRUB_DIR)/grub.cfg
	@echo "Creating ISO image..."
	@if [ ! -f "$(GRUB_DIR)/grub.cfg" ]; then \
		echo "Error: grub.cfg not found"; \
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

$(INITRD_ROOT)/bin:
	mkdir -p $(INITRD_ROOT)/bin
	mkdir -p $(INITRD_ROOT)/etc
	mkdir -p $(INITRD_ROOT)/drivers

# Create virtual HDD for testing
.PHONY: create-hdd
create-hdd:
	@chmod +x scripts/create_hdd.sh
	@./scripts/create_hdd.sh

# Run in VirtualBox with HDD
.PHONY: run-vbox
run-vbox: $(ISO_FILE)
	@chmod +x scripts/run_virtualbox.sh
	@./scripts/run_virtualbox.sh $(ISO_FILE)

# Run in VirtualBox with HDD for installation testing
.PHONY: run-vbox-hdd
run-vbox-hdd: $(ISO_FILE) create-hdd
	@chmod +x scripts/run_virtualbox.sh
	@./scripts/run_virtualbox.sh $(ISO_FILE) --with-hdd

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(KERNEL_ELF) $(INITRD_CPIO) $(ISO_FILE)
	rm -f $(INITRD_ROOT)/bin/*.elf
	@echo "Clean complete"

# Deep clean
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
	@echo "  toolchain-check- Verify required tools"
	@echo "  kernel         - Build kernel.elf"
	@echo "  libsys         - Build system API library"
	@echo "  apps           - Build all applications"
	@echo "  initrd         - Create initrd.cpio"
	@echo "  iso            - Create bootable ISO"
	@echo "  create-hdd     - Create virtual hard disk"
	@echo "  run-vbox       - Run in VirtualBox (CD-ROM)"
	@echo "  run-vbox-hdd   - Run in VirtualBox with HDD"
	@echo "  clean          - Remove build artifacts"
	@echo "  distclean      - Remove all generated files"
	@echo "  help           - Show this help"
	@echo ""
	@echo "Version: $(VERSION)"