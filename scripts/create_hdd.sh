#!/bin/bash
# create_hdd.sh - Create a virtual hard disk for VirtualBox testing

set -e

HDD_FILE="ramOS.vdi"
HDD_SIZE=8192  # 8GB in MB

# Check if file already exists
if [ -f "$HDD_FILE" ]; then
    echo "Hard disk already exists: $HDD_FILE"
    read -p "Recreate? This will delete all data! (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Keeping existing disk"
        exit 0
    fi
    
    echo "Removing old disk..."
    
    # Close any VirtualBox medium that might be using it
    VBoxManage closemedium disk "$HDD_FILE" &> /dev/null || true
    
    rm -f "$HDD_FILE"
fi

echo "Creating virtual hard disk..."
echo "  File: $HDD_FILE"
echo "  Size: ${HDD_SIZE}MB (8GB)"
echo ""

# Create new VDI disk
VBoxManage createmedium disk --filename "$HDD_FILE" --size $HDD_SIZE --format VDI

echo ""
echo "✓ Virtual hard disk created successfully!"
echo ""
echo "The HDD will be automatically attached when you run:"
echo "  make run-vbox"
echo ""
echo "To install ramOS to the HDD:"
echo "  1. Run: make run-vbox"
echo "  2. Boot from the ISO (installer)"
echo "  3. In ramOS shell, run: /bin/installer"
echo "  4. Follow the installation prompts"
echo "  5. Reboot and it will boot from the installed system"