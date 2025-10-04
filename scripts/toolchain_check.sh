#!/bin/bash
# toolchain_check.sh - Verify and auto-install required tools

# Don't use set -e here because we want to continue checking all tools
# even if some are missing

echo "Checking build toolchain..."

# Detect OS
if [ -f /etc/arch-release ]; then
    OS="arch"
    echo "Detected: Arch Linux"
elif [ -f /etc/debian_version ]; then
    OS="debian"
    echo "Detected: Debian/Ubuntu"
else
    OS="unknown"
    echo "Detected: Unknown OS"
fi

MISSING=0
MISSING_TOOLS=()

check_tool() {
    if command -v "$1" &> /dev/null; then
        echo "  [✓] $1 found"
        return 0
    else
        echo "  [✗] $1 NOT FOUND"
        MISSING=1
        MISSING_TOOLS+=("$1")
        return 1
    fi
}

# Required tools
check_tool "nasm" || true
check_tool "i686-elf-gcc" || true
check_tool "i686-elf-ld" || true
check_tool "i686-elf-ar" || true
check_tool "grub-mkrescue" || true
check_tool "xorriso" || true
check_tool "cpio" || true
check_tool "mformat" || true

# Optional tools
echo ""
echo "Optional tools (for VirtualBox support):"
VBOX_MISSING=0
if ! check_tool "VBoxManage"; then
    VBOX_MISSING=1
fi

echo ""
if [ $MISSING -eq 1 ]; then
    echo "=========================================="
    echo "ERROR: Some required tools are missing!"
    echo "=========================================="
    echo ""
    echo "Missing tools: ${MISSING_TOOLS[*]}"
    echo ""
    
    if [ "$OS" = "arch" ]; then
        echo "🔧 Arch Linux detected - I can auto-install these for you!"
        echo ""
        echo "Would you like to auto-install missing dependencies? [Y/n]"
        read -r response
        
        # Default to yes if empty
        response=${response:-y}
        
        if [[ "$response" =~ ^[Yy]$ ]] || [[ -z "$response" ]]; then
            echo ""
            echo "========================================"
            echo "Installing missing dependencies..."
            echo "========================================"
            
            # Install base tools
            PACMAN_PACKAGES=()
            YAY_PACKAGES=()
            
            for tool in "${MISSING_TOOLS[@]}"; do
                case "$tool" in
                    nasm)
                        PACMAN_PACKAGES+=("nasm")
                        ;;
                    grub-mkrescue)
                        PACMAN_PACKAGES+=("grub" "mtools")
                        ;;
                    xorriso)
                        PACMAN_PACKAGES+=("libisoburn")
                        ;;
                    cpio)
                        PACMAN_PACKAGES+=("cpio")
                        ;;
                    mformat)
                        PACMAN_PACKAGES+=("mtools")
                        ;;
                    i686-elf-gcc|i686-elf-ld|i686-elf-ar)
                        if [[ ! " ${YAY_PACKAGES[@]} " =~ " i686-elf-binutils " ]]; then
                            YAY_PACKAGES+=("i686-elf-binutils")
                        fi
                        if [[ ! " ${YAY_PACKAGES[@]} " =~ " i686-elf-gcc " ]]; then
                            YAY_PACKAGES+=("i686-elf-gcc")
                        fi
                        ;;
                esac
            done
            
            # Install with pacman
            if [ ${#PACMAN_PACKAGES[@]} -gt 0 ]; then
                echo ""
                echo "📦 Installing with pacman: ${PACMAN_PACKAGES[*]}"
                if ! sudo pacman -S --needed --noconfirm "${PACMAN_PACKAGES[@]}"; then
                    echo "❌ Failed to install pacman packages"
                    exit 1
                fi
                echo "✓ Pacman packages installed"
            fi
            
            # Install with yay
            if [ ${#YAY_PACKAGES[@]} -gt 0 ]; then
                echo ""
                if command -v yay &> /dev/null; then
                    echo "📦 Installing with yay: ${YAY_PACKAGES[*]}"
                    echo "⚠️  This may take 20-30 minutes (building GCC from source)..."
                    if ! yay -S --needed --noconfirm "${YAY_PACKAGES[@]}"; then
                        echo "❌ Failed to install AUR packages"
                        exit 1
                    fi
                    echo "✓ AUR packages installed"
                else
                    echo "⚠️  yay not found. Installing yay first..."
                    if ! sudo pacman -S --needed --noconfirm base-devel git; then
                        echo "❌ Failed to install yay dependencies"
                        exit 1
                    fi
                    
                    cd /tmp
                    rm -rf yay
                    if ! git clone https://aur.archlinux.org/yay.git; then
                        echo "❌ Failed to clone yay repository"
                        exit 1
                    fi
                    cd yay
                    if ! makepkg -si --noconfirm; then
                        echo "❌ Failed to build yay"
                        exit 1
                    fi
                    cd - > /dev/null
                    
                    echo "✓ yay installed"
                    echo ""
                    echo "📦 Installing with yay: ${YAY_PACKAGES[*]}"
                    echo "⚠️  This may take 20-30 minutes (building GCC from source)..."
                    if ! yay -S --needed --noconfirm "${YAY_PACKAGES[@]}"; then
                        echo "❌ Failed to install AUR packages"
                        exit 1
                    fi
                    echo "✓ AUR packages installed"
                fi
            fi
            
            echo ""
            echo "=========================================="
            echo "✅ Installation complete!"
            echo "=========================================="
            echo ""
            echo "Re-checking toolchain..."
            echo ""
            
            # Re-run check
            exec "$0"
        else
            echo ""
            echo "Installation cancelled."
            echo ""
            echo "To install manually:"
            echo "  sudo pacman -S nasm grub mtools libisoburn cpio"
            echo "  yay -S i686-elf-binutils i686-elf-gcc"
            echo ""
            exit 1
        fi
    elif [ "$OS" = "debian" ]; then
        echo "Ubuntu/Debian installation commands:"
        echo "  sudo apt-get install nasm grub-pc-bin grub-common xorriso cpio mtools"
        echo "  # For i686-elf toolchain, see: https://wiki.osdev.org/GCC_Cross-Compiler"
        echo ""
        echo "Optional (VirtualBox):"
        echo "  sudo apt-get install virtualbox"
        exit 1
    else
        echo "Unknown OS. Please install manually:"
        echo "  - nasm, grub-mkrescue, xorriso, cpio, mtools"
        echo "  - i686-elf cross-compiler toolchain"
        echo "  - VirtualBox (optional)"
        exit 1
    fi
else
    echo "=========================================="
    echo "✅ All required tools are available!"
    echo "=========================================="
    
    if [ $VBOX_MISSING -eq 1 ] && [ "$OS" = "arch" ]; then
        echo ""
        echo "Note: VirtualBox is not installed (optional)"
        echo "To install: sudo pacman -S virtualbox virtualbox-host-modules-arch"
    fi
    
    # Create mtools config if it doesn't exist (fixes grub-mkrescue issue)
    if [ ! -f "$HOME/.mtoolsrc" ]; then
        echo ""
        echo "Creating ~/.mtoolsrc to fix grub-mkrescue issues..."
        cat > "$HOME/.mtoolsrc" << 'EOF'
mtools_skip_check=1
EOF
        echo "✓ Created ~/.mtoolsrc"
    fi
    
    exit 0
fi