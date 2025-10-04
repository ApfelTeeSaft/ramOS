#!/bin/bash
# create_or_update_vbox_vm.sh - Create or update VirtualBox VM for ramOS

set -e

VM_NAME="ramOS"
ISO_PATH="$1"

if [ -z "$ISO_PATH" ]; then
    echo "Usage: $0 <iso-path>"
    exit 1
fi

# Convert to absolute path
ISO_PATH=$(realpath "$ISO_PATH")

echo "Setting up VirtualBox VM: $VM_NAME"
echo "ISO: $ISO_PATH"

# Check if VM exists
if VBoxManage showvminfo "$VM_NAME" &> /dev/null; then
    echo "VM '$VM_NAME' exists, updating..."
    
    # Power off VM if running
    if VBoxManage showvminfo "$VM_NAME" | grep -q "running"; then
        echo "Powering off VM..."
        VBoxManage controlvm "$VM_NAME" poweroff || true
        sleep 2
    fi
else
    echo "Creating new VM '$VM_NAME'..."
    
    # Create VM
    VBoxManage createvm --name "$VM_NAME" --ostype Other --register
    
    # Configure VM
    VBoxManage modifyvm "$VM_NAME" \
        --memory 256 \
        --boot1 dvd \
        --boot2 none \
        --boot3 none \
        --boot4 none \
        --firmware bios \
        --ioapic off \
        --rtcuseutc on
    
    # Create storage controller
    VBoxManage storagectl "$VM_NAME" \
        --name "IDE" \
        --add ide \
        --controller PIIX4 \
        --bootable on
fi

# Detach any existing DVD
VBoxManage storageattach "$VM_NAME" \
    --storagectl "IDE" \
    --port 0 \
    --device 0 \
    --medium none &> /dev/null || true

# Attach ISO
echo "Attaching ISO..."
VBoxManage storageattach "$VM_NAME" \
    --storagectl "IDE" \
    --port 0 \
    --device 0 \
    --type dvddrive \
    --medium "$ISO_PATH"

echo "VM '$VM_NAME' is ready!"
echo ""
echo "To start: VBoxManage startvm $VM_NAME"
echo "To start headless: VBoxManage startvm $VM_NAME --type headless"