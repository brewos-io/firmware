#!/bin/bash

# Helper script to update GitHub links in the firmware repo after extraction
# This updates references from old repo names to the new structure

set -e

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Updating GitHub links in firmware repository..."

cd "${BASE_DIR}"

# Update links from old repo to new org/repo structure
find . -type f \( -name "*.md" -o -name "*.ts" -o -name "*.tsx" -o -name "*.js" -o -name "*.jsx" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.py" \) \
    -not -path "./node_modules/*" \
    -not -path "./.git/*" \
    -not -path "./build/*" \
    -not -path "./dist/*" \
    -not -path "./.pio/*" \
    -exec sed -i.bak \
        -e 's|github.com/mizrachiran/brewos|github.com/brewos-io/firmware|g' \
        -e 's|github.com/brewos-io/all|github.com/brewos-io/firmware|g' \
        {} \; 2>/dev/null || true

# Clean up backup files
find . -name "*.bak" -delete 2>/dev/null || true

echo "✓ GitHub links updated"
echo ""
echo "Review changes with: git diff"
echo "Commit with: git add -A && git commit -m 'Update GitHub repository links'"

