#!/bin/bash
# run_vbox.sh - Run ramOS in VirtualBox

set -e

ISO_PATH="${1:-iso/ramOS.iso}"

if [ ! -f "$ISO_PATH" ]; then
    echo "Error: ISO file not found: $ISO_PATH"
    echo "Run 'make iso' to build the ISO first"
    exit 1
fi

# Create/update VM (will automatically attach HDD if it exists)
./scripts/create_or_update_vbox_vm.sh "$ISO_PATH"

# Start VM
echo ""
echo "Starting VM..."
VBoxManage startvm ramOS

echo ""
echo "VM started."
echo ""
echo "To power off:"
echo "  VBoxManage controlvm ramOS poweroff"