#!/bin/bash
# setup_project.sh - Create all necessary directories and files

set -e

# Get the script's directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Setting up ramOS project structure..."
echo "Project root: $PROJECT_ROOT"
echo ""

# Create directories
echo "Creating directories..."
mkdir -p "$PROJECT_ROOT/build"
mkdir -p "$PROJECT_ROOT/iso/boot/grub"
mkdir -p "$PROJECT_ROOT/src/boot"
mkdir -p "$PROJECT_ROOT/src/kernel"
mkdir -p "$PROJECT_ROOT/initrd_root"
mkdir -p "$PROJECT_ROOT/.github/workflows"

# Create grub.cfg if it doesn't exist
GRUB_CFG="$PROJECT_ROOT/iso/boot/grub/grub.cfg"
if [ ! -f "$GRUB_CFG" ]; then
    echo "Creating grub.cfg..."
    cat > "$GRUB_CFG" << 'EOF'
set timeout=3
set default=0

menuentry "ramOS" {
    multiboot /boot/kernel.elf
    module /boot/initrd.cpio
    boot
}
EOF
    echo "✓ Created $GRUB_CFG"
else
    echo "✓ grub.cfg already exists"
fi

# Create initrd files if they don't exist
if [ ! -f "$PROJECT_ROOT/initrd_root/README.txt" ]; then
    echo "Creating initrd_root/README.txt..."
    cat > "$PROJECT_ROOT/initrd_root/README.txt" << 'EOF'
ramOS initrd README
===================

This is the initial RAM disk for ramOS. All files here are loaded into
memory at boot time and made available through a simple read-only filesystem.

Files in this initrd:
- README.txt - This file
- motd.txt   - Message of the day

You can list these files with the 'ls' command and view them with 'cat'.

To add more files to the initrd:
1. Place files in the initrd_root/ directory
2. Run 'make initrd iso' to rebuild
3. The new files will be available after booting

ramOS is a minimal hobby operating system for learning purposes.
EOF
    echo "✓ Created initrd_root/README.txt"
else
    echo "✓ initrd_root/README.txt already exists"
fi

if [ ! -f "$PROJECT_ROOT/initrd_root/motd.txt" ]; then
    echo "Creating initrd_root/motd.txt..."
    cat > "$PROJECT_ROOT/initrd_root/motd.txt" << 'EOF'
╔════════════════════════════════════════════════════════════╗
║                                                            ║
║              Welcome to ramOS - A Hobby OS!                ║
║                                                            ║
║  This is a minimal operating system built for learning     ║
║  operating system development concepts.                    ║
║                                                            ║
║  Features:                                                 ║
║  • 32-bit protected mode kernel                            ║
║  • Multiboot v1 boot via GRUB2                             ║
║  • Interrupt handling (IDT, ISR, IRQ)                      ║
║  • PIT timer for uptime tracking                           ║
║  • PS/2 keyboard input                                     ║
║  • VGA text mode console                                   ║
║  • RAM-based filesystem (initrd)                           ║
║  • Interactive shell with built-in commands                ║
║                                                            ║
║  Type 'help' to see available commands.                    ║
║                                                            ║
╚════════════════════════════════════════════════════════════╝
EOF
    echo "✓ Created initrd_root/motd.txt"
else
    echo "✓ initrd_root/motd.txt already exists"
fi

# Make all scripts executable
echo ""
echo "Making scripts executable..."
chmod +x "$SCRIPT_DIR"/*.sh
echo "✓ All scripts are now executable"

echo ""
echo "=========================================="
echo "✅ Project setup complete!"
echo "=========================================="
echo ""
echo "Directory structure:"
echo "  $PROJECT_ROOT/build/           - Build artifacts (created)"
echo "  $PROJECT_ROOT/iso/boot/grub/   - GRUB config (created)"
echo "  $PROJECT_ROOT/initrd_root/     - initrd files (created)"
echo ""
echo "Next steps:"
echo "  1. Ensure all source files are in place"
echo "  2. Run: make"
echo "  3. Run: make run-vbox"
echo ""