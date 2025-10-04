#!/bin/bash
# run_virtualbox.sh - Run ramOS in VirtualBox

set -e

ISO_PATH="$1"

if [ -z "$ISO_PATH" ]; then
    echo "Usage: $0 <iso-path>"
    exit 1
fi

if [ ! -f "$ISO_PATH" ]; then
    echo "Error: ISO file not found: $ISO_PATH"
    exit 1
fi

# Create/update VM
./scripts/create_or_update_vbox_vm.sh "$ISO_PATH"

# Start VM
echo ""
echo "Starting VM..."
VBoxManage startvm ramOS --type headless

echo ""
echo "VM started in headless mode."
echo "To view console output, use VirtualBox GUI or:"
echo "  VBoxManage controlvm ramOS screenshotpng /tmp/ramos_screenshot.png"
echo ""
echo "To power off:"
echo "  VBoxManage controlvm ramOS poweroff"