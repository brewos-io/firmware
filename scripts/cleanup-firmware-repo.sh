#!/bin/bash

# Script to clean up the firmware repo after extracting web and homeassistant
# This will:
# 1. Remove pages/ and homeassistant/ directories
# 2. Remove pages.yml workflow
# 3. Update .gitignore
# 4. Update README if needed

set -e

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Cleaning Up Firmware Repository ===${NC}\n"

# Check if we're in a git repo
if [ ! -d "${BASE_DIR}/.git" ]; then
    echo -e "${RED}Error: Not a git repository${NC}"
    exit 1
fi

# Check if we're on main branch
CURRENT_BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null || echo "unknown")
if [ "$CURRENT_BRANCH" != "main" ] && [ "$CURRENT_BRANCH" != "master" ]; then
    echo -e "${YELLOW}Warning: Not on main/master branch (currently on: ${CURRENT_BRANCH})${NC}"
    read -p "Continue anyway? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
fi

# Check if directories exist
if [ ! -d "${BASE_DIR}/pages" ] && [ ! -d "${BASE_DIR}/homeassistant" ]; then
    echo -e "${YELLOW}Warning: pages/ and homeassistant/ directories not found${NC}"
    echo "They may have already been removed."
    read -p "Continue anyway? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
fi

cd "${BASE_DIR}"

# Show what will be removed
echo -e "${YELLOW}The following will be removed:${NC}"
[ -d "pages" ] && echo "  - pages/"
[ -d "homeassistant" ] && echo "  - homeassistant/"
[ -f ".github/workflows/pages.yml" ] && echo "  - .github/workflows/pages.yml"
echo ""
echo -e "${BLUE}Note:${NC} The assets/ directory will be kept in the firmware repo"
echo "      (it's also used by the firmware and has been copied to the web repo)"
echo ""

read -p "Continue with cleanup? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 0
fi

# Remove directories
if [ -d "pages" ]; then
    echo "Removing pages/ directory..."
    git rm -rf pages/ 2>/dev/null || rm -rf pages/
fi

if [ -d "homeassistant" ]; then
    echo "Removing homeassistant/ directory..."
    git rm -rf homeassistant/ 2>/dev/null || rm -rf homeassistant/
fi

# Remove pages workflow
if [ -f ".github/workflows/pages.yml" ]; then
    echo "Removing pages.yml workflow..."
    git rm .github/workflows/pages.yml 2>/dev/null || rm .github/workflows/pages.yml
fi

# Update .gitignore - remove pages/assets line
if [ -f ".gitignore" ]; then
    echo "Updating .gitignore..."
    # Remove the pages/assets line if it exists
    sed -i.bak '/^# GitHub Pages assets (copied during build)$/d' .gitignore
    sed -i.bak '/^pages\/assets$/d' .gitignore
    rm -f .gitignore.bak
    git add .gitignore
fi

# Update README to reference new repos
if [ -f "README.md" ]; then
    echo "Checking README for updates..."
    if ! grep -q "brewos-io/web" README.md 2>/dev/null; then
        echo -e "${YELLOW}Consider updating README.md to reference the new repositories:${NC}"
        echo "  - https://github.com/brewos-io/web"
        echo "  - https://github.com/brewos-io/homeassistant"
    fi
fi

# Commit changes
echo ""
echo "Staging changes..."
git add -A

if git diff --cached --quiet; then
    echo -e "${YELLOW}No changes to commit${NC}"
else
    echo "Committing changes..."
    git commit -m "Remove pages and homeassistant directories (moved to separate repositories)

- pages/ → brewos-io/web
- homeassistant/ → brewos-io/homeassistant" || echo -e "${YELLOW}No changes to commit${NC}"
fi

echo -e "\n${GREEN}=== Cleanup Complete ===${NC}"
echo ""
echo "Changes have been committed locally."
echo "Review with: git log -1"
echo "Push with: git push"
echo ""
read -p "Push changes now? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git push
    echo -e "${GREEN}✓ Pushed to remote${NC}"
fi

