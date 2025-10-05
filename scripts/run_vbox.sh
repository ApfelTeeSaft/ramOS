#!/bin/bash
# run_vbox.sh - Run ramOS in VirtualBox

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

ISO_PATH="${1:-$PROJECT_ROOT/iso/ramOS.iso}"

if [ ! -f "$ISO_PATH" ]; then
    echo "Error: ISO file not found: $ISO_PATH"
    echo "Run 'make iso' to build the ISO first"
    exit 1
fi

# Create/update VM (will automatically attach HDD if it exists)
"$SCRIPT_DIR/create_or_update_vbox_vm.sh" "$ISO_PATH"

# Start VM
echo ""
echo "Starting VM..."
VBoxManage startvm ramOS

echo ""
echo "VM started."
echo ""
echo "To power off:"
echo "  VBoxManage controlvm ramOS poweroff"