#!/bin/bash
# create_or_update_vbox_vm.sh - Create or update VirtualBox VM for ramOS

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

VM_NAME="ramOS"
ISO_PATH="${1:-$PROJECT_ROOT/iso/ramOS.iso}"

# Convert to absolute path
ISO_PATH=$(realpath "$ISO_PATH")

# Check if ISO exists
if [ ! -f "$ISO_PATH" ]; then
    echo "Error: ISO file not found: $ISO_PATH"
    echo "Run 'make iso' to build the ISO first"
    exit 1
fi

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
    
    # Check if SATA controller exists, create if not
    if ! VBoxManage showvminfo "$VM_NAME" | grep -q "SATA"; then
        echo "Adding SATA controller to existing VM..."
        VBoxManage storagectl "$VM_NAME" \
            --name "SATA" \
            --add sata \
            --controller IntelAhci \
            --portcount 1 \
            --bootable on
    fi
else
    echo "Creating new VM '$VM_NAME'..."
    
    # Create VM
    VBoxManage createvm --name "$VM_NAME" --ostype Other --register
    
    # Configure VM
    VBoxManage modifyvm "$VM_NAME" \
        --memory 256 \
        --boot1 dvd \
        --boot2 disk \
        --boot3 none \
        --boot4 none \
        --firmware bios \
        --ioapic off \
        --rtcuseutc on
    
    # Create IDE controller
    VBoxManage storagectl "$VM_NAME" \
        --name "IDE" \
        --add ide \
        --controller PIIX4 \
        --bootable on
    
    # Create SATA controller for HDD
    VBoxManage storagectl "$VM_NAME" \
        --name "SATA" \
        --add sata \
        --controller IntelAhci \
        --portcount 1 \
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

# Attach HDD if it exists
HDD_FILE="$PROJECT_ROOT/ramOS.vdi"
if [ -f "$HDD_FILE" ]; then
    echo "Found HDD: $HDD_FILE"
    echo "Attaching HDD to SATA controller..."
    
    # Detach any existing HDD first
    VBoxManage storageattach "$VM_NAME" \
        --storagectl "SATA" \
        --port 0 \
        --device 0 \
        --medium none &> /dev/null || true
    
    # Attach the HDD
    VBoxManage storageattach "$VM_NAME" \
        --storagectl "SATA" \
        --port 0 \
        --device 0 \
        --type hdd \
        --medium "$HDD_FILE"
    
    echo "HDD attached successfully"
else
    echo "No HDD found ($HDD_FILE). Run 'make create-hdd' to create one."
fi

echo ""
echo "VM '$VM_NAME' is ready!"
echo ""
echo "Boot order: 1. DVD (installer), 2. HDD (installed system)"
echo ""
echo "To start: VBoxManage startvm $VM_NAME"
echo "To start headless: VBoxManage startvm $VM_NAME --type headless"