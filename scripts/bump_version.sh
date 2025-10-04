#!/bin/bash
# bump_version.sh - Bump version number and update changelog

set -e

BUMP_TYPE="${1:-patch}"
VERSION_FILE="VERSION"
CHANGELOG_FILE="CHANGELOG.md"
MANIFEST_FILE="manifest.yaml"

if [ ! -f "$VERSION_FILE" ]; then
    echo "Error: VERSION file not found"
    exit 1
fi

# Read current version
CURRENT_VERSION=$(cat "$VERSION_FILE")
echo "Current version: $CURRENT_VERSION"

# Parse version
IFS='.' read -r -a VERSION_PARTS <<< "$CURRENT_VERSION"
MAJOR="${VERSION_PARTS[0]}"
MINOR="${VERSION_PARTS[1]}"
PATCH="${VERSION_PARTS[2]}"

# Bump version
case "$BUMP_TYPE" in
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    patch)
        PATCH=$((PATCH + 1))
        ;;
    *)
        echo "Usage: $0 [major|minor|patch]"
        exit 1
        ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
echo "New version: $NEW_VERSION"

# Update VERSION file
echo "$NEW_VERSION" > "$VERSION_FILE"

# Update manifest.yaml
if [ -f "$MANIFEST_FILE" ]; then
    sed -i.bak "s/^version: .*/version: $NEW_VERSION/" "$MANIFEST_FILE"
    rm -f "${MANIFEST_FILE}.bak"
fi

# Update CHANGELOG
if [ -f "$CHANGELOG_FILE" ]; then
    DATE=$(date +%Y-%m-%d)
    
    # Create temporary file with new entry
    {
        echo "# Changelog"
        echo ""
        echo "All notable changes to ramOS will be documented in this file."
        echo ""
        echo "The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),"
        echo "and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)."
        echo ""
        echo "## [$NEW_VERSION] - $DATE"
        echo ""
        echo "### Added"
        echo "- "
        echo ""
        echo "### Changed"
        echo "- "
        echo ""
        echo "### Fixed"
        echo "- "
        echo ""
        tail -n +7 "$CHANGELOG_FILE"
    } > "${CHANGELOG_FILE}.tmp"
    
    mv "${CHANGELOG_FILE}.tmp" "$CHANGELOG_FILE"
fi

echo ""
echo "Version bumped to $NEW_VERSION"
echo "Please edit $CHANGELOG_FILE to add release notes"
echo ""
echo "To create git tag:"
echo "  git add VERSION $MANIFEST_FILE $CHANGELOG_FILE"
echo "  git commit -m \"Bump version to $NEW_VERSION\""
echo "  git tag v$NEW_VERSION"
echo "  git push && git push --tags"