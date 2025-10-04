#!/bin/bash
# make_initrd.sh - Create initrd cpio archive

set -e

# Get the script's directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

INITRD_ROOT="$PROJECT_ROOT/initrd_root"
OUTPUT="$PROJECT_ROOT/iso/boot/initrd.cpio"

echo "Creating initrd from ${INITRD_ROOT}/..."

# Check if initrd_root exists
if [ ! -d "$INITRD_ROOT" ]; then
    echo "Error: initrd_root directory not found at $INITRD_ROOT"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$(dirname "$OUTPUT")"

# Create cpio archive (newc format, uncompressed for simplicity)
cd "$INITRD_ROOT"
find . -type f | cpio -o -H newc > "$OUTPUT" 2>/dev/null
cd - > /dev/null

SIZE=$(stat -c%s "$OUTPUT" 2>/dev/null || stat -f%z "$OUTPUT" 2>/dev/null)
echo "initrd created: $OUTPUT ($SIZE bytes)"