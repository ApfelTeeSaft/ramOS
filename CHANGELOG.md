# Changelog

All notable changes to ramOS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-10-04

### Added
- Initial release of ramOS
- Multiboot v1 compliant bootloader (NASM)
- 32-bit protected mode kernel in C
- GDT and IDT initialization
- Interrupt handling (ISR, IRQ)
- PIT timer for uptime tracking
- VGA text mode console with color support
- PS/2 keyboard driver with line input
- RAM-based filesystem (initrd via cpio newc format)
- Interactive shell with built-in commands:
  - `help` - List available commands
  - `mem` - Display memory statistics
  - `uptime` - Show system uptime
  - `echo` - Print text to console
  - `ls` - List files in initrd
  - `cat` - Display file contents
  - `sysinfo` - Show CPU and kernel information
- Build system with Makefile
- VirtualBox automation scripts
- GitHub Actions CI/CD pipeline
- Automated release creation with ISO artifacts

### Technical Details
- Architecture: i686 (32-bit x86)
- Bootloader: GRUB2 (Multiboot v1)
- Kernel entry at 0x00100000
- Freestanding C environment
- Simple bump allocator for heap
- Read-only initrd filesystem

[0.1.0]: https://github.com/username/ramOS/releases/tag/v0.1.0