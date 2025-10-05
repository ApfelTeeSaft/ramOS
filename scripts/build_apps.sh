#!/bin/bash
# build_apps.sh - Build all applications

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================"
echo "Building ramOS Applications"
echo "========================================"
echo ""

cd "$PROJECT_ROOT"

# Check if Makefile exists
if [ ! -f "Makefile" ]; then
    echo "Error: Makefile not found"
    exit 1
fi

# Build system API library first
echo "[1/2] Building system API library..."
make libsys

# Build all applications
echo "[2/2] Building applications..."
make apps

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo ""
echo "Applications built:"
ls -lh initrd_root/bin/*.elf 2>/dev/null || echo "  (none found)"

echo ""
echo "To create bootable ISO:"
echo "  make initrd iso"