#!/bin/bash

# VirtualBox Kernel Module Auto-Compiler for Arch Linux
# This script recompiles and loads VirtualBox kernel modules

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root or with sudo${NC}"
    exit 1
fi

echo -e "${GREEN}=== VirtualBox Kernel Module Setup ===${NC}\n"

# Get current kernel version
KERNEL_VERSION=$(uname -r)
echo -e "${YELLOW}Current kernel version:${NC} $KERNEL_VERSION"

# Check if VirtualBox is installed
if ! command -v VBoxManage &> /dev/null; then
    echo -e "${RED}Error: VirtualBox is not installed${NC}"
    exit 1
fi

# Check if virtualbox-host-modules-arch or virtualbox-host-dkms is installed
echo -e "\n${YELLOW}Checking VirtualBox host modules...${NC}"

if pacman -Qi virtualbox-host-modules-arch &> /dev/null; then
    echo "Found: virtualbox-host-modules-arch"
    MODULE_PKG="virtualbox-host-modules-arch"
elif pacman -Qi virtualbox-host-dkms &> /dev/null; then
    echo "Found: virtualbox-host-dkms"
    MODULE_PKG="virtualbox-host-dkms"
else
    echo -e "${RED}Error: No VirtualBox host modules package found${NC}"
    echo "Please install either 'virtualbox-host-modules-arch' or 'virtualbox-host-dkms'"
    echo "Example: sudo pacman -S virtualbox-host-modules-arch"
    exit 1
fi

# Ensure linux-headers for CURRENT kernel are installed
echo -e "\n${YELLOW}Checking linux-headers for current kernel...${NC}"

# First, ensure the linux-headers package is installed
if ! pacman -Qi linux-headers &> /dev/null; then
    echo -e "${YELLOW}Installing linux-headers...${NC}"
    pacman -S --noconfirm linux-headers
else
    echo "linux-headers package installed"
fi

# Check what kernel version linux-headers provides
HEADERS_VERSION=$(pacman -Ql linux-headers 2>/dev/null | grep '/usr/lib/modules/' | head -1 | cut -d'/' -f5)

if [ -z "$HEADERS_VERSION" ]; then
    echo -e "${YELLOW}Could not detect headers version from package, checking filesystem...${NC}"
    HEADERS_VERSION=$(ls /usr/lib/modules/ | grep -v "$(uname -r)" | head -1)
    if [ -z "$HEADERS_VERSION" ]; then
        HEADERS_VERSION="$KERNEL_VERSION"
    fi
fi

echo -e "${YELLOW}Headers installed for kernel:${NC} $HEADERS_VERSION"

# Check if they match
if [ "$KERNEL_VERSION" != "$HEADERS_VERSION" ]; then
    echo -e "${RED}Kernel version mismatch!${NC}"
    echo -e "Running kernel: ${YELLOW}$KERNEL_VERSION${NC}"
    echo -e "Headers for:    ${YELLOW}$HEADERS_VERSION${NC}"
    echo ""
    echo -e "${YELLOW}Solutions:${NC}"
    echo "1. Reboot to use the latest kernel ($HEADERS_VERSION)"
    echo "2. Or update your system: sudo pacman -Syu"
    echo ""
    read -p "Try to build for running kernel anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# If using DKMS, ensure dkms is installed and rebuild
if [ "$MODULE_PKG" == "virtualbox-host-dkms" ]; then
    echo -e "\n${YELLOW}Ensuring DKMS is installed...${NC}"
    if ! command -v dkms &> /dev/null; then
        pacman -S --noconfirm dkms
    fi
    
    # Get VirtualBox version - DKMS uses the full version with _OSE suffix
    VBOX_VERSION=$(pacman -Q virtualbox-host-dkms | awk '{print $2}' | cut -d'-' -f1)
    VBOX_DKMS_VERSION="${VBOX_VERSION}_OSE"
    echo -e "${YELLOW}VirtualBox version:${NC} $VBOX_VERSION"
    echo -e "${YELLOW}DKMS module version:${NC} $VBOX_DKMS_VERSION"
    
    # Check if DKMS source directory exists (with _OSE suffix)
    if [ ! -d "/usr/src/vboxhost-$VBOX_DKMS_VERSION" ]; then
        echo -e "${RED}DKMS source directory missing: /usr/src/vboxhost-$VBOX_DKMS_VERSION${NC}"
        echo -e "${YELLOW}Reinstalling virtualbox-host-dkms...${NC}"
        pacman -S --noconfirm virtualbox-host-dkms
        
        # Verify it exists now
        if [ ! -d "/usr/src/vboxhost-$VBOX_DKMS_VERSION" ]; then
            echo -e "${RED}Source directory still missing after reinstall!${NC}"
            echo -e "${YELLOW}Checking /usr/src for vboxhost directories...${NC}"
            ls -la /usr/src/ | grep vbox || echo "No vboxhost directories found"
            exit 1
        fi
    fi
    
    # Check DKMS status
    echo -e "\n${YELLOW}Checking DKMS status...${NC}"
    dkms status vboxhost 2>/dev/null || echo "No existing vboxhost modules in DKMS"
    
    # Check if module is registered but broken
    if dkms status | grep -q "vboxhost/$VBOX_DKMS_VERSION.*installed"; then
        echo -e "${YELLOW}Module shows as installed but may be misconfigured${NC}"
        echo -e "${YELLOW}Completely removing from DKMS and re-adding...${NC}"
        
        # Remove from all kernels
        dkms remove vboxhost/$VBOX_DKMS_VERSION --all 2>/dev/null || true
        
        # Re-add the module to DKMS
        echo -e "${YELLOW}Re-adding module to DKMS...${NC}"
        dkms add vboxhost/$VBOX_DKMS_VERSION 2>/dev/null || echo "Module already added or add failed"
    fi
    
    # Remove old vboxhost modules if they exist for this kernel
    echo -e "\n${YELLOW}Cleaning old DKMS modules for this kernel...${NC}"
    dkms remove vboxhost/$VBOX_DKMS_VERSION -k $KERNEL_VERSION 2>/dev/null || true
    
    # Build and install for current kernel
    echo -e "\n${YELLOW}Building VirtualBox DKMS modules for kernel $KERNEL_VERSION...${NC}"
    if dkms install vboxhost/$VBOX_DKMS_VERSION -k $KERNEL_VERSION; then
        echo -e "${GREEN}✓ DKMS modules built successfully${NC}"
    else
        echo -e "${RED}✗ Failed to build DKMS modules${NC}"
        echo -e "\n${YELLOW}Checking if we need to create the kernel build symlink...${NC}"
        
        # Check if the build directory exists but symlink is wrong
        if [ -d "/usr/lib/modules/$HEADERS_VERSION/build" ] && [ "$KERNEL_VERSION" != "$HEADERS_VERSION" ]; then
            echo -e "${YELLOW}Creating symlink from running kernel to installed headers...${NC}"
            mkdir -p "/usr/lib/modules/$KERNEL_VERSION"
            ln -sf "/usr/lib/modules/$HEADERS_VERSION/build" "/usr/lib/modules/$KERNEL_VERSION/build"
            
            echo -e "${YELLOW}Retrying DKMS build...${NC}"
            if ! dkms install vboxhost/$VBOX_DKMS_VERSION -k $KERNEL_VERSION; then
                echo -e "${RED}Build failed again. Please update your system or reboot to the latest kernel.${NC}"
                exit 1
            fi
        else
            exit 1
        fi
    fi
    
else
    # For non-DKMS package (virtualbox-host-modules-arch)
    echo -e "\n${YELLOW}Using virtualbox-host-modules-arch${NC}"
    echo "This package should provide pre-built modules for the Arch kernel."
    echo "If modules aren't loading, you may need to update: sudo pacman -Syu"
fi

# Load the modules
echo -e "\n${YELLOW}Loading VirtualBox kernel modules...${NC}"

# Unload modules first if they're already loaded (stale versions)
modprobe -r vboxnetadp 2>/dev/null || true
modprobe -r vboxnetflt 2>/dev/null || true
modprobe -r vboxdrv 2>/dev/null || true

# Load modules
if ! modprobe vboxdrv; then
    echo -e "${RED}Failed to load vboxdrv module${NC}"
    echo -e "${YELLOW}Checking dmesg for errors...${NC}"
    dmesg | tail -20 | grep -i vbox || true
    exit 1
fi

modprobe vboxnetflt 2>/dev/null || echo "vboxnetflt not loaded (optional)"
modprobe vboxnetadp 2>/dev/null || echo "vboxnetadp not loaded (optional)"

# Verify modules are loaded
echo -e "\n${YELLOW}Verifying modules are loaded...${NC}"
if lsmod | grep -q vboxdrv; then
    echo -e "${GREEN}✓ vboxdrv loaded successfully${NC}"
else
    echo -e "${RED}✗ vboxdrv failed to load${NC}"
    exit 1
fi

if lsmod | grep -q vboxnetflt; then
    echo -e "${GREEN}✓ vboxnetflt loaded successfully${NC}"
else
    echo -e "${YELLOW}⚠ vboxnetflt not loaded (optional, needed for bridged networking)${NC}"
fi

if lsmod | grep -q vboxnetadp; then
    echo -e "${GREEN}✓ vboxnetadp loaded successfully${NC}"
else
    echo -e "${YELLOW}⚠ vboxnetadp not loaded (optional, needed for host-only networking)${NC}"
fi

# Add user to vboxusers group if not already
echo -e "\n${YELLOW}Checking vboxusers group membership...${NC}"
if [ -n "$SUDO_USER" ]; then
    if ! groups "$SUDO_USER" | grep -q vboxusers; then
        echo -e "${YELLOW}Adding $SUDO_USER to vboxusers group...${NC}"
        usermod -aG vboxusers "$SUDO_USER"
        echo -e "${GREEN}✓ Added $SUDO_USER to vboxusers group${NC}"
        echo -e "${YELLOW}Note: You may need to log out and back in for group changes to take effect${NC}"
    else
        echo -e "${GREEN}✓ $SUDO_USER is already in vboxusers group${NC}"
    fi
fi

echo -e "\n${GREEN}=== Setup Complete ===${NC}"
echo -e "VirtualBox kernel modules have been compiled and loaded successfully."
echo -e "You should now be able to start VMs with: ${YELLOW}VBoxManage startvm <vm-name>${NC}\n"