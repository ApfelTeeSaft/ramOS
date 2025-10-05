#!/bin/bash
# update_manifest.sh - Update application manifest with build time

set -e

MANIFEST="$1"

if [ ! -f "$MANIFEST" ]; then
    echo "Manifest not found: $MANIFEST"
    exit 1
fi

# Get current timestamp
BUILD_TIME=$(date -u +"%Y-%m-%d %H:%M:%S UTC")

# Update build_time in manifest
if grep -q "^build_time:" "$MANIFEST"; then
    # Update existing build_time
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "s/^build_time:.*/build_time: \"$BUILD_TIME\"/" "$MANIFEST"
    else
        sed -i "s/^build_time:.*/build_time: \"$BUILD_TIME\"/" "$MANIFEST"
    fi
else
    # Add build_time
    echo "build_time: \"$BUILD_TIME\"" >> "$MANIFEST"
fi

# Copy manifest to initrd
MANIFEST_NAME=$(basename "$MANIFEST")
APP_NAME=$(basename $(dirname "$MANIFEST"))
mkdir -p initrd_root/etc/manifests
cp "$MANIFEST" "initrd_root/etc/manifests/${APP_NAME}.yaml"