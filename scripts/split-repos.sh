#!/bin/bash

# Script to split the monorepo into separate repositories
# Usage: ./scripts/split-repos.sh

set -e

ORG="brewos-io"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP_DIR="${BASE_DIR}/.split-temp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== BrewOS Repository Splitter ===${NC}\n"

# Check if we're in the right directory
if [ ! -d "${BASE_DIR}/.git" ]; then
    echo -e "${RED}Error: Not a git repository${NC}"
    exit 1
fi

# Function to create a new repo from a subtree
create_repo_from_subtree() {
    local repo_name=$1
    local source_path=$2
    local description=$3
    
    echo -e "\n${YELLOW}Creating ${repo_name} repository...${NC}"
    
    if [ ! -d "${source_path}" ]; then
        echo -e "${RED}Warning: ${source_path} does not exist, skipping${NC}"
        return
    fi
    
    # Create temporary directory for the new repo
    local temp_repo="${TEMP_DIR}/${repo_name}"
    mkdir -p "${temp_repo}"
    
    # Clone the current repo
    echo "  Cloning current repository..."
    git clone "${BASE_DIR}" "${temp_repo}" --no-hardlinks
    
    cd "${temp_repo}"
    
    # Create a new branch for filtering
    git checkout -b split-branch
    
    # Use git filter-branch to keep only the desired directory
    echo "  Extracting ${source_path}..."
    
    # Move the directory to root
    if [ "${source_path}" != "." ]; then
        git filter-branch --prune-empty --subdirectory-filter "${source_path}" split-branch
        
        # Clean up
        git for-each-ref --format="%(refname)" refs/original/ | xargs -n 1 git update-ref -d 2>/dev/null || true
        git reflog expire --expire=now --all
        git gc --prune=now --aggressive
    fi
    
    # Check if remote already exists
    if git ls-remote --exit-code "https://github.com/${ORG}/${repo_name}.git" &>/dev/null; then
        echo -e "${YELLOW}  Repository ${ORG}/${repo_name} already exists on GitHub${NC}"
        read -p "  Do you want to push to it? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git remote add origin "https://github.com/${ORG}/${repo_name}.git" || git remote set-url origin "https://github.com/${ORG}/${repo_name}.git"
            echo "  Pushing to GitHub..."
            git push -u origin split-branch:main --force || git push -u origin split-branch:master --force
        fi
    else
        echo -e "${GREEN}  Repository ${ORG}/${repo_name} does not exist yet${NC}"
        echo "  Create it on GitHub first, then run this script again"
        echo "  Or manually push:"
        echo "    cd ${temp_repo}"
        echo "    git remote add origin https://github.com/${ORG}/${repo_name}.git"
        echo "    git push -u origin split-branch:main"
    fi
    
    cd "${BASE_DIR}"
    
    echo -e "${GREEN}  ✓ ${repo_name} prepared in ${temp_repo}${NC}"
}

# Function to create repo using git subtree (simpler, but less history preservation)
create_repo_with_subtree() {
    local repo_name=$1
    local source_path=$2
    local description=$3
    
    echo -e "\n${YELLOW}Creating ${repo_name} repository using subtree...${NC}"
    
    if [ ! -d "${source_path}" ]; then
        echo -e "${RED}Warning: ${source_path} does not exist, skipping${NC}"
        return
    fi
    
    # Create temporary directory for the new repo
    local temp_repo="${TEMP_DIR}/${repo_name}"
    mkdir -p "${temp_repo}"
    
    # Initialize new repo
    cd "${temp_repo}"
    git init
    git checkout -b main
    
    # Add the original repo as a remote
    git remote add source "${BASE_DIR}"
    git fetch source
    
    # Extract subtree
    echo "  Extracting ${source_path}..."
    git subtree pull --prefix="${source_path}" source main --squash || \
    git subtree pull --prefix="${source_path}" source master --squash || true
    
    # Create initial commit if needed
    if [ -z "$(git log --oneline 2>/dev/null)" ]; then
        echo "  Creating initial commit..."
        git add .
        git commit -m "Initial commit: Extract ${source_path} from monorepo" || true
    fi
    
    # Check if remote already exists
    if git ls-remote --exit-code "https://github.com/${ORG}/${repo_name}.git" &>/dev/null; then
        echo -e "${YELLOW}  Repository ${ORG}/${repo_name} already exists on GitHub${NC}"
        read -p "  Do you want to push to it? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git remote add origin "https://github.com/${ORG}/${repo_name}.git" || git remote set-url origin "https://github.com/${ORG}/${repo_name}.git"
            echo "  Pushing to GitHub..."
            git push -u origin main --force || git push -u origin master --force
        fi
    else
        echo -e "${GREEN}  Repository ${ORG}/${repo_name} does not exist yet${NC}"
        echo "  Create it on GitHub first, then run this script again"
        echo "  Or manually push:"
        echo "    cd ${temp_repo}"
        echo "    git remote add origin https://github.com/${ORG}/${repo_name}.git"
        echo "    git push -u origin main"
    fi
    
    cd "${BASE_DIR}"
    
    echo -e "${GREEN}  ✓ ${repo_name} prepared in ${temp_repo}${NC}"
}

# Clean up temp directory
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    read -p "Remove temporary directories? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf "${TEMP_DIR}"
        echo -e "${GREEN}  ✓ Cleaned up${NC}"
    else
        echo -e "${YELLOW}  Temporary repos kept in ${TEMP_DIR}${NC}"
    fi
}

# Main execution
main() {
    echo "This script will help you split the monorepo into separate repositories."
    echo "Make sure you have created the repositories on GitHub first:"
    echo "  - ${ORG}/firmware"
    echo "  - ${ORG}/web (or ${ORG}/pages)"
    echo "  - ${ORG}/homeassistant"
    echo ""
    read -p "Continue? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
    
    mkdir -p "${TEMP_DIR}"
    
    # Ask which method to use
    echo ""
    echo "Choose extraction method:"
    echo "  1) git filter-branch (preserves full history, slower)"
    echo "  2) git subtree (simpler, may lose some history)"
    read -p "Enter choice (1 or 2) [default: 2]: " method
    method=${method:-2}
    
    # Ask about pages vs web name
    echo ""
    read -p "Name for pages repo? (web/pages) [default: web]: " pages_name
    pages_name=${pages_name:-web}
    
    if [ "$method" = "1" ]; then
        # Method 1: Full history preservation with filter-branch
        echo -e "\n${GREEN}Using git filter-branch method (full history)${NC}"
        
        # Firmware repo (src/esp32, src/pico, src/shared, src/web, src/cloud, src/scripts)
        # Note: This is complex with multiple directories, so we'll do it differently
        echo -e "\n${YELLOW}For firmware, we'll need to handle multiple directories manually${NC}"
        echo "Consider using git filter-repo for better results:"
        echo "  pip install git-filter-repo"
        echo "  git filter-repo --path src/esp32 --path src/pico --path src/shared --path src/web --path src/cloud --path src/scripts --to-subdirectory-filter firmware"
        
        # For now, let's do the simpler ones
        create_repo_from_subtree "${pages_name}" "pages" "GitHub Pages site"
        create_repo_from_subtree "homeassistant" "homeassistant" "Home Assistant integration"
    else
        # Method 2: Simpler subtree method
        echo -e "\n${GREEN}Using git subtree method${NC}"
        
        # For firmware, we need a different approach since it spans multiple directories
        echo -e "\n${YELLOW}Note: Firmware spans multiple directories (src/esp32, src/pico, etc.)${NC}"
        echo "You may want to manually create the firmware repo or use git filter-repo"
        echo ""
        
        create_repo_with_subtree "${pages_name}" "pages" "GitHub Pages site"
        create_repo_with_subtree "homeassistant" "homeassistant" "Home Assistant integration"
    fi
    
    cleanup
}

main

