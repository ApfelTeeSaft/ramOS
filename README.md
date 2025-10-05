# ramOS - Advanced Edition

![Version](https://img.shields.io/badge/version-0.2.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Architecture](https://img.shields.io/badge/arch-i686-orange)

A hobby operating system with **userspace applications**, **system API**, **driver framework**, **EXT4 filesystem support**, and a complete **system installer**. Built for learning OS development concepts.

## 🎯 Features

### Core System
- **Multiboot v1 compliant** bootloader (GRUB2)
- **32-bit protected mode** kernel in C
- **Global Descriptor Table** (GDT) and **Interrupt Descriptor Table** (IDT)
- **Hardware interrupt handling** (ISR/IRQ)
- **PIT timer** for uptime tracking (100Hz)
- **VGA text mode** console with color support
- **PS/2 keyboard** driver

### Filesystem & Storage
- **EXT4 filesystem** implementation
- **Virtual File System** (VFS) layer
- **ATA/IDE disk driver** (VirtualBox optimized)
- **Partition table support** (MBR)
- **RAM-based initrd** (cpio format)

### Driver Framework
- **Loadable driver architecture**
- **Block device** drivers (ATA/IDE)
- **USB support** framework (UHCI controller)
- **PCI bus** enumeration
- **Device file interface** (/dev)

### Userspace & Applications
- **System API library** (libsys) for applications
- **Process management** (fork, exec, wait)
- **System call interface** (INT 0x80)
- **5 built-in applications**:
  - **Calculator** - Arithmetic calculator
  - **Text Editor** - Line-based text editor
  - **File Manager** - Browse and manage files
  - **Shell** - Command-line interface with process execution
  - **Installer** - Install ramOS to hard disk

### Installation System
- **Disk partitioning** utility
- **EXT4 formatting** tool
- **GRUB installation** to MBR
- **System file deployment**
- **Bootable hard disk** creation

## 📋 System Requirements

### Build Dependencies
- **NASM** (Netwide Assembler)
- **i686-elf cross-compiler** (binutils + gcc)
- **GRUB utilities** (grub-mkrescue)
- **xorriso**
- **cpio**
- **make**

### Runtime (VirtualBox)
- **VirtualBox** 6.0 or later
- **256MB RAM** minimum
- **8GB virtual HDD** (for installation)

## 🚀 Quick Start

### 1. Install Dependencies

**Arch Linux:**
```bash
# Automated installation
make toolchain-check  # Will offer to install missing dependencies

# Or manual:
sudo pacman -S nasm grub mtools libisoburn cpio
yay -S i686-elf-binutils i686-elf-gcc
```

**Ubuntu/Debian:**
```bash
sudo apt-get install nasm grub-pc-bin grub-common xorriso cpio mtools build-essential
# For i686-elf toolchain: https://wiki.osdev.org/GCC_Cross-Compiler
```

### 2. Build ramOS

```bash
git clone https://github.com/yourusername/ramOS.git
cd ramOS

# Build everything (kernel + apps + ISO)
make

# Build artifacts:
# - iso/boot/kernel.elf
# - iso/boot/initrd.cpio
# - iso/ramOS.iso
# - initrd_root/bin/*.elf (applications)
```

### 3. Run in VirtualBox

**Live CD Mode (no installation):**
```bash
make run-vbox
```

**With Virtual HDD (for installer testing):**
```bash
make create-hdd      # Create 8GB virtual disk
make run-vbox-hdd    # Boot with HDD attached
```

### 4. Install to Hard Disk

Once booted:
```
ramOS> /bin/installer

# Follow the installer prompts to:
# 1. Partition the disk
# 2. Format as EXT4
# 3. Install system files
# 4. Install GRUB bootloader
```

## 🎮 Applications

### Calculator
```bash
/bin/calculator

# Commands:
# - Enter numbers and operators (+, -, *, /)
# - = to calculate
# - c to clear
# - q to quit
```

### Text Editor
```bash
/bin/editor [filename]

# Commands:
# :w [file]  - Save file
# :q         - Quit
# :wq        - Save and quit
# :l         - List lines
# :d [line]  - Delete line
# <text>     - Append line
```

### File Manager
```bash
/bin/filemanager

# Commands:
# ls          - List directory
# cd <dir>    - Change directory
# cat <file>  - View file
# mkdir <dir> - Create directory
# rm <file>   - Delete file
# stat <file> - File information
```

### Shell
```bash
/bin/shell

# Features:
# - Execute applications from /bin
# - Built-in commands: cd, pwd, help, exit
# - Process management (fork/exec)
```

## 📁 Project Structure

```
ramOS/
├── src/
│   ├── boot/
│   │   └── boot.asm              # Multiboot bootloader
│   ├── kernel/
│   │   ├── core/                 # Core kernel (GDT, IDT, ISR, IRQ, timer)
│   │   ├── mm/                   # Memory management
│   │   ├── drivers/              # Device drivers (console, keyboard, ATA, USB)
│   │   ├── fs/                   # Filesystem (VFS, EXT4, initrd)
│   │   ├── proc/                 # Process management
│   │   ├── api/                  # System call implementations
│   │   └── shell.c               # Built-in kernel shell
│   ├── api/
│   │   ├── libsys.c/h            # System API library
│   │   └── libsys.ld             # Library linker script
│   ├── apps/
│   │   ├── common/
│   │   │   ├── app_start.c       # Application startup code
│   │   │   └── app.ld            # Application linker script
│   │   ├── calculator/
│   │   ├── editor/
│   │   ├── filemanager/
│   │   ├── installer/
│   │   └── shell/
│   └── linker.ld                 # Kernel linker script
├── scripts/
│   ├── toolchain_check.sh        # Auto-install dependencies (Arch)
│   ├── make_initrd.sh            # Create initrd archive
│   ├── update_manifest.sh        # Update app manifests
│   ├── create_hdd.sh             # Create virtual HDD
│   └── run_virtualbox.sh         # Run in VirtualBox
├── initrd_root/
│   ├── bin/                      # Application binaries
│   ├── etc/
│   │   └── manifests/            # Application metadata
│   └── drivers/                  # Loadable drivers
├── Makefile                      # Complete build system
└── README.md
```

## 🔧 Development

### Building Individual Components

```bash
make kernel          # Build kernel only
make libsys          # Build system API library
make apps            # Build all applications
make initrd          # Create initrd archive
make iso             # Create bootable ISO
```

### Adding a New Application

1. **Create application directory:**
```bash
mkdir -p src/apps/myapp
```

2. **Write your application:**
```c
// src/apps/myapp/myapp.c
#include "../../api/libsys.h"

int main(int argc, char* argv[]) {
    println("Hello from MyApp!");
    return 0;
}
```

3. **Create manifest:**
```yaml
# src/apps/myapp/manifest.yaml
name: MyApp
description: My custom application
version: 1.0.0
author: Your Name
category: Utility
binary: myapp.elf
permissions:
  - console_io
```

4. **Add to Makefile:**
```makefile
# Add to APP_NAMES
APP_NAMES = calculator editor filemanager installer shell myapp

# Add build rule
$(INITRD_ROOT)/bin/myapp.elf: $(APPS_DIR)/myapp/myapp.c $(LIBSYS) $(APP_LINKER)
	$(CC) $(APP_CFLAGS) -c $(APPS_DIR)/myapp/myapp.c -o $(BUILD_DIR)/myapp.o
	$(LD) $(APP_LDFLAGS) -o $@ $(BUILD_DIR)/myapp.o $(LIBSYS)
```

5. **Build:**
```bash
make apps
make initrd iso
```

### System API Functions

Applications can use these system calls:

**Process Management:**
- `sys_exit(code)` - Exit process
- `sys_fork()` - Create child process
- `sys_exec(path, argv)` - Execute program
- `sys_wait(status)` - Wait for child

**File I/O:**
- `sys_open(path, flags)` - Open file
- `sys_close(fd)` - Close file
- `sys_read(fd, buf, count)` - Read from file
- `sys_write(fd, buf, count)` - Write to file
- `sys_seek(fd, offset, whence)` - Seek in file

**Directory Operations:**
- `sys_readdir(fd, entry)` - Read directory entry
- `sys_mkdir(path, mode)` - Create directory
- `sys_chdir(path)` - Change directory
- `sys_getcwd(buf, size)` - Get current directory

**Helper Functions:**
- `print(str)`, `println(str)` - Console output
- `printf(fmt, ...)` - Formatted output
- `readln(buf, max)` - Read line input
- String utilities: `strlen`, `strcmp`, `strcpy`, `memcpy`, etc.

## 🏗️ Architecture

### Memory Layout
```
0x00000000 - 0x000FFFFF: Real mode memory (1MB)
0x00100000 - 0x001FFFFF: Kernel code + data
0x00200000+            : Kernel heap
0x10000000+            : User applications
0xB8000                : VGA text buffer
```

### System Call Interface
Applications invoke system calls using INT 0x80:
```
EAX = syscall number
EBX = arg1
ECX = arg2
EDX = arg3
Return value in EAX
```

### Driver Framework
Drivers implement the `driver_ops_t` interface:
- `init()` - Initialize driver
- `open(minor)` - Open device
- `read/write(minor, buf, count, offset)` - I/O operations
- `ioctl(minor, cmd, arg)` - Device-specific commands

### Boot Sequence
1. **BIOS** → **GRUB** (loads kernel + initrd)
2. **boot.asm** (multiboot entry, sets up stack)
3. **kmain()** (initialize kernel subsystems)
4. **Load drivers** (ATA, console, keyboard)
5. **Mount initrd** (unpack applications)
6. **Start shell** (user interaction)

## 📝 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

- [OSDev.org](https://osdev.org) community
- GRUB bootloader developers
- VirtualBox team
- All OS development resources

## 📧 Contact

For questions, please open an issue on GitHub.

---

**Current Version**: 0.2.0  
**Built with**: NASM, GCC, GRUB, VirtualBox, and ❤️