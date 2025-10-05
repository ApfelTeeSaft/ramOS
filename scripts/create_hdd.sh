#!/bin/bash
# create_hdd.sh - Create a virtual hard disk for VirtualBox testing

set -e

HDD_FILE="ramOS.vdi"
HDD_SIZE=8192  # 8GB in MB

echo "Creating virtual hard disk..."
echo "  File: $HDD_FILE"
echo "  Size: ${HDD_SIZE}MB"

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
    rm -f "$HDD_FILE"
fi

# Create new VDI disk
VBoxManage createmedium disk --filename "$HDD_FILE" --size $HDD_SIZE --format VDI

echo "Virtual hard disk created successfully!"
echo ""
echo "To attach to VM, use:"
echo "  VBoxManage storageattach ramOS --storagectl SATA --port 0 --device 0 --type hdd --medium $HDD_FILE"