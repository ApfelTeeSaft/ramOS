# ramOS

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Architecture](https://img.shields.io/badge/arch-i686-orange)

A minimal hobby operating system that boots via GRUB2 (Multiboot v1) into a 32-bit C kernel with a RAM-based filesystem and interactive shell. Built for learning OS development concepts.

## 🎯 Features

- **Multiboot v1 compliant** bootloader (NASM assembly)
- **32-bit protected mode** kernel in C
- **Global Descriptor Table** (GDT) setup
- **Interrupt Descriptor Table** (IDT) with ISR/IRQ handling
- **PIT timer** for uptime tracking (100Hz)
- **VGA text mode** console with color support
- **PS/2 keyboard** driver with line input
- **RAM-based filesystem** (initrd via cpio newc format)
- **Interactive shell** with 7 built-in commands
- **Automated CI/CD** with GitHub Actions
- **VirtualBox automation** scripts included

## 📋 System Requirements

### Build Dependencies

- **NASM** (Netwide Assembler)
- **i686-elf cross-compiler** (binutils + gcc)
- **GRUB utilities** (grub-mkrescue)
- **xorriso**
- **cpio**
- **make**

### Optional

- **VirtualBox** (for testing)

## 🚀 Quick Start

### 1. Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install nasm xorriso grub-pc-bin grub-common cpio build-essential
```

**Building the i686-elf cross-compiler:**
```bash
# Download and build binutils and gcc for i686-elf target
# See: https://wiki.osdev.org/GCC_Cross-Compiler
```

### 2. Clone and Build

```bash
git clone https://github.com/yourusername/ramOS.git
cd ramOS

# Check toolchain
make toolchain-check

# Build everything
make

# Build artifacts will be at:
# - iso/boot/kernel.elf
# - iso/boot/initrd.cpio
# - iso/ramOS.iso
```

### 3. Run in VirtualBox

```bash
# Automated (creates/updates VM and starts it)
make run-vbox

# Manual
./scripts/create_or_update_vbox_vm.sh iso/ramOS.iso
VBoxManage startvm ramOS
```

### 4. Test in QEMU (alternative)

```bash
qemu-system-i386 -cdrom iso/ramOS.iso -m 256M
```

## 🎮 Shell Commands

Once booted, the interactive shell supports these commands:

| Command | Description |
|---------|-------------|
| `help` | Show list of available commands |
| `mem` | Display memory statistics (total/used/free) |
| `uptime` | Show system uptime in hours/minutes/seconds |
| `echo <text>` | Print text to console |
| `ls` | List all files in initrd |
| `cat <file>` | Display contents of a file from initrd |
| `sysinfo` | Show CPU vendor, kernel version, and system info |

### Example Session

```
ramOS> help
Available commands:
  help     - Show this help message
  mem      - Display memory statistics
  uptime   - Show system uptime
  echo     - Print text to console
  ls       - List files in initrd
  cat      - Display file contents
  sysinfo  - Show system information

ramOS> sysinfo
ramOS System Information
  Kernel Version: 0.1.0
  Architecture:   i686 (32-bit)
  CPU Vendor:     GenuineIntel
  CPU Mode:       Protected Mode
  Bootloader:     Multiboot v1 (GRUB2)

ramOS> ls
Files in initrd (2 total):
  README.txt                     512 bytes
  motd.txt                       856 bytes

ramOS> cat motd.txt
[displays welcome message]

ramOS> mem
Memory Statistics:
  Total: 256000 KB
  Used:  128 KB
  Free:  255872 KB

ramOS> uptime
Uptime: 0 hours, 2 minutes, 34 seconds
Ticks: 15400 (ms: 154000)
```

## 📁 Project Structure

```
ramOS/
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions CI/CD
├── src/
│   ├── boot/
│   │   └── boot.asm            # Multiboot bootloader (NASM)
│   ├── kernel/
│   │   ├── kmain.c             # Kernel entry point
│   │   ├── gdt.c/h             # Global Descriptor Table
│   │   ├── idt.c/h             # Interrupt Descriptor Table
│   │   ├── isr.c/h             # Interrupt Service Routines
│   │   ├── irq.c/h             # Hardware interrupt handling
│   │   ├── pit.c/h             # Programmable Interval Timer
│   │   ├── timer.c/h           # System timer
│   │   ├── memory.c/h          # Memory management
│   │   ├── console.c/h         # VGA text mode driver
│   │   ├── keyboard.c/h        # PS/2 keyboard driver
│   │   ├── initrd.c/h          # initrd filesystem
│   │   └── shell.c/h           # Interactive shell
│   └── linker.ld               # Linker script
├── scripts/
│   ├── toolchain_check.sh      # Verify build tools
│   ├── make_initrd.sh          # Create initrd archive
│   ├── bump_version.sh         # Version management
│   ├── create_or_update_vbox_vm.sh
│   └── run_virtualbox.sh
├── initrd_root/
│   ├── README.txt              # Initrd documentation
│   └── motd.txt                # Welcome message
├── iso/
│   └── boot/
│       └── grub/
│           └── grub.cfg        # GRUB configuration
├── Makefile                    # Build system
├── VERSION                     # Current version (0.1.0)
├── manifest.yaml               # Project metadata
├── CHANGELOG.md                # Version history
└── README.md                   # This file
```

## 🔧 Development

### Adding Files to initrd

1. Place files in `initrd_root/` directory:
   ```bash
   echo "Hello from ramOS!" > initrd_root/hello.txt
   ```

2. Rebuild initrd and ISO:
   ```bash
   make initrd iso
   ```

3. Files will be accessible via `ls` and `cat` commands after booting

### Modifying the Kernel

1. Edit source files in `src/kernel/`
2. Rebuild:
   ```bash
   make clean
   make
   ```

3. Test changes:
   ```bash
   make run-vbox
   ```

### Version Management

The project uses [Semantic Versioning](https://semver.org/):

```bash
# Bump patch version (0.1.0 -> 0.1.1)
./scripts/bump_version.sh patch

# Bump minor version (0.1.0 -> 0.2.0)
./scripts/bump_version.sh minor

# Bump major version (0.1.0 -> 1.0.0)
./scripts/bump_version.sh major
```

This updates `VERSION`, `manifest.yaml`, and `CHANGELOG.md`.

### Make Targets

| Target | Description |
|--------|-------------|
| `make` | Build everything (default) |
| `make toolchain-check` | Verify required tools |
| `make kernel` | Build kernel.elf only |
| `make initrd` | Create initrd.cpio |
| `make iso` | Create bootable ISO |
| `make run-vbox` | Run in VirtualBox |
| `make clean` | Remove build artifacts |
| `make distclean` | Deep clean (including ISO) |
| `make help` | Show all targets |

## 🏗️ Architecture Details

### Memory Layout

```
0x00000000 - 0x000FFFFF: Real mode memory (1MB)
0x00100000 - 0x001FFFFF: Kernel code + data
0x00200000+            : Heap (bump allocator)
0xB8000                : VGA text buffer
```

### Boot Sequence

1. **BIOS** loads **GRUB** from disk
2. **GRUB** reads `grub.cfg` and loads:
   - `kernel.elf` (Multiboot ELF binary)
   - `initrd.cpio` (as a module)
3. **GRUB** jumps to kernel entry (`boot.asm`)
4. **boot.asm** sets up stack and calls `kmain()`
5. **kmain()** initializes:
   - GDT (flat memory model)
   - IDT (interrupt handlers)
   - PIC (remap IRQs to INT 32-47)
   - PIT (100Hz timer)
   - Memory subsystem
   - Keyboard driver
   - initrd filesystem
6. **Shell** starts and waits for user input

### Interrupt Handling

- **ISR 0-31**: CPU exceptions (divide by zero, page fault, etc.)
- **IRQ 0 (INT 32)**: PIT timer (100Hz)
- **IRQ 1 (INT 33)**: PS/2 keyboard
- **IRQ 2-15**: Available for future hardware

### Filesystem

The initrd is a **cpio newc** format archive:
- Loaded into RAM by GRUB as a module
- Parsed at boot time
- Provides read-only access to files
- No disk I/O required

## 🚢 CI/CD Pipeline

GitHub Actions automatically:

1. **On push to `main`**:
   - Sets up Ubuntu build environment
   - Builds i686-elf cross-compiler (cached)
   - Compiles kernel and creates ISO
   - Uploads artifacts

2. **Creates GitHub Release**:
   - Tag: `v{VERSION}` (from VERSION file)
   - Attaches: `ramOS.iso`, `kernel.elf`, `initrd.cpio`
   - Includes instructions and changelog

### Manual Release

```bash
# 1. Bump version
./scripts/bump_version.sh minor

# 2. Edit CHANGELOG.md with release notes

# 3. Commit and tag
git add VERSION manifest.yaml CHANGELOG.md
git commit -m "Release v0.2.0"
git tag v0.2.0

# 4. Push (triggers CI)
git push && git push --tags
```

## 🎓 Educational Notes

This OS is designed for learning purposes and has several intentional limitations:

### What's Included
✅ Protected mode (32-bit)  
✅ Interrupt handling  
✅ Basic memory management  
✅ Device drivers (keyboard, timer, VGA)  
✅ Simple filesystem (read-only)  
✅ Interactive shell  

### What's NOT Included
❌ User mode / privilege separation  
❌ Virtual memory / paging  
❌ Dynamic memory allocation (free/realloc)  
❌ Multitasking / scheduling  
❌ Disk drivers (ATA/AHCI)  
❌ Filesystem writes  
❌ Networking  
❌ USB support  

### Learning Resources

- [OSDev Wiki](https://wiki.osdev.org/) - Comprehensive OS development guide
- [JamesM's Kernel Tutorial](http://www.jamesmolloy.co.uk/tutorial_html/) - Step-by-step tutorial
- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [Intel Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## 🐛 Troubleshooting

### Build Errors

**Problem**: `i686-elf-gcc: command not found`  
**Solution**: Install i686-elf cross-compiler. See [GCC Cross-Compiler Guide](https://wiki.osdev.org/GCC_Cross-Compiler).

**Problem**: `grub-mkrescue: command not found`  
**Solution**: Install GRUB utilities: `sudo apt-get install grub-pc-bin grub-common`

### Runtime Issues

**Problem**: Kernel doesn't boot / black screen  
**Solution**: 
- Check multiboot header is correct
- Verify GRUB config paths match actual files
- Try QEMU with `-d int,cpu_reset` for debugging

**Problem**: Keyboard doesn't work  
**Solution**:
- Ensure IRQs are enabled (`sti` instruction)
- Check PIC remapping is correct
- Verify keyboard IRQ handler is registered

**Problem**: Triple fault / system reset loop  
**Solution**:
- Check GDT/IDT are properly initialized
- Verify stack is set up correctly
- Use QEMU with `-d int` to see interrupts

## 📝 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🤝 Contributing

This is an educational project, but contributions are welcome!

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 🙏 Acknowledgments

- [OSDev.org](https://osdev.org) community
- GRUB bootloader developers
- All the tutorials and resources that made this possible

## 📧 Contact

For questions or suggestions, please open an issue on GitHub.

---

**Current Version**: 0.1.0  
**Built with**: NASM, GCC, GRUB, and ❤️