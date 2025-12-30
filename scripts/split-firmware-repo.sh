#!/bin/bash

# Script to extract firmware-related directories into a separate repo
# This handles multiple directories: src/esp32, src/pico, src/shared, src/web, src/cloud, src/scripts

set -e

ORG="brewos-io"
REPO_NAME="firmware"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP_DIR="${BASE_DIR}/.split-temp/${REPO_NAME}"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}=== Extracting Firmware Repository ===${NC}\n"

# Check for git-filter-repo
if command -v git-filter-repo &> /dev/null; then
    echo -e "${GREEN}✓ git-filter-repo found${NC}\n"
    USE_FILTER_REPO=true
else
    echo -e "${YELLOW}⚠ git-filter-repo not found${NC}"
    echo "  Install it with: pip install git-filter-repo"
    echo "  Or we'll use an alternative method\n"
    USE_FILTER_REPO=false
fi

mkdir -p "${TEMP_DIR}"

# Clone the repo
echo "Cloning repository..."
git clone "${BASE_DIR}" "${TEMP_DIR}" --no-hardlinks
cd "${TEMP_DIR}"

if [ "$USE_FILTER_REPO" = true ]; then
    # Method 1: Using git-filter-repo (best method)
    echo -e "\n${GREEN}Using git-filter-repo to extract directories...${NC}"
    
    # Extract multiple paths and move them to root
    git filter-repo \
        --path src/esp32 \
        --path src/pico \
        --path src/shared \
        --path src/web \
        --path src/cloud \
        --path src/scripts \
        --path-rename src/esp32:esp32 \
        --path-rename src/pico:pico \
        --path-rename src/shared:shared \
        --path-rename src/web:web \
        --path-rename src/cloud:cloud \
        --path-rename src/scripts:scripts \
        --path assets \
        --path docs \
        --path LICENSE \
        --path README.md \
        --path SETUP.md \
        --path CONTRIBUTING.md \
        --path CODE_OF_CONDUCT.md \
        --path TESTERS.md \
        --path VERSION \
        --path version.json \
        --path cliff.toml \
        --path .gitignore \
        --force
    
    echo -e "${GREEN}✓ Extraction complete${NC}"
else
    # Method 2: Manual extraction (simpler but loses some history)
    echo -e "\n${YELLOW}Using manual extraction method...${NC}"
    
    # Create a new branch
    git checkout -b main 2>/dev/null || git checkout main
    
    # Remove everything except what we want
    echo "Removing unwanted files..."
    git rm -rf --ignore-unmatch \
        pages \
        homeassistant \
        scripts/split-repos.sh \
        scripts/split-firmware-repo.sh \
        2>/dev/null || true
    
    # Move src/* directories to root
    if [ -d "src/esp32" ]; then
        git mv src/esp32 . 2>/dev/null || mv src/esp32 .
    fi
    if [ -d "src/pico" ]; then
        git mv src/pico . 2>/dev/null || mv src/pico .
    fi
    if [ -d "src/shared" ]; then
        git mv src/shared . 2>/dev/null || mv src/shared .
    fi
    if [ -d "src/web" ]; then
        git mv src/web . 2>/dev/null || mv src/web .
    fi
    if [ -d "src/cloud" ]; then
        git mv src/cloud . 2>/dev/null || mv src/cloud .
    fi
    if [ -d "src/scripts" ]; then
        git mv src/scripts . 2>/dev/null || mv src/scripts .
    fi
    
    # Remove empty src directory
    rmdir src 2>/dev/null || true
    
    # Commit changes
    git add -A
    git commit -m "Extract firmware directories from monorepo" || true
    
    echo -e "${GREEN}✓ Extraction complete${NC}"
fi

# Show what we have
echo -e "\n${GREEN}Repository structure:${NC}"
ls -la | head -20

# Check if remote exists
echo -e "\n${YELLOW}Checking GitHub repository...${NC}"
if git ls-remote --exit-code "https://github.com/${ORG}/${REPO_NAME}.git" &>/dev/null; then
    echo -e "${GREEN}✓ Repository ${ORG}/${REPO_NAME} exists on GitHub${NC}"
    read -p "Push to GitHub? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git remote add origin "https://github.com/${ORG}/${REPO_NAME}.git" 2>/dev/null || \
        git remote set-url origin "https://github.com/${ORG}/${REPO_NAME}.git"
        
        # Determine default branch
        DEFAULT_BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null || echo "main")
        echo "Pushing to ${DEFAULT_BRANCH}..."
        git push -u origin "${DEFAULT_BRANCH}" --force
        echo -e "${GREEN}✓ Pushed to GitHub${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Repository ${ORG}/${REPO_NAME} does not exist on GitHub${NC}"
    echo "Create it first, then run:"
    echo "  cd ${TEMP_DIR}"
    echo "  git remote add origin https://github.com/${ORG}/${REPO_NAME}.git"
    echo "  git push -u origin main"
fi

echo -e "\n${GREEN}✓ Firmware repository ready in: ${TEMP_DIR}${NC}"

