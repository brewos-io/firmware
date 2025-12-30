#!/bin/bash

# Simple script to extract single-directory repos (web/pages and homeassistant)
# Usage: ./scripts/split-simple-repos.sh [web|homeassistant|both]

set -e

ORG="brewos-io"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP_DIR="${BASE_DIR}/.split-temp"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

REPO_TO_SPLIT=${1:-both}

extract_repo() {
    local repo_name=$1
    local source_path=$2
    
    echo -e "\n${GREEN}=== Extracting ${repo_name} ===${NC}"
    
    if [ ! -d "${BASE_DIR}/${source_path}" ]; then
        echo -e "${RED}Error: ${source_path} does not exist${NC}"
        return 1
    fi
    
    local temp_repo="${TEMP_DIR}/${repo_name}"
    mkdir -p "${temp_repo}"
    
    # Check for git-filter-repo
    if command -v git-filter-repo &> /dev/null; then
        echo "Using git-filter-repo (best method)..."
        
        # Clone and filter
        git clone "${BASE_DIR}" "${temp_repo}" --no-hardlinks
        cd "${temp_repo}"
        
        git filter-repo --path "${source_path}" --path-rename "${source_path}:." --force
        
        echo -e "${GREEN}✓ Extracted with full history${NC}"
    else
        echo "Using git subtree method..."
        
        # Initialize new repo
        cd "${temp_repo}"
        git init
        git checkout -b main
        
        # Add source as remote
        git remote add source "${BASE_DIR}"
        git fetch source
        
        # Extract subtree
        DEFAULT_BRANCH=$(cd "${BASE_DIR}" && git symbolic-ref --short HEAD 2>/dev/null || echo "main")
        git subtree pull --prefix="${source_path}" source "${DEFAULT_BRANCH}" --squash || \
        git subtree pull --prefix="${source_path}" source master --squash || true
        
        # If subtree pull didn't work, manually copy
        if [ -z "$(git log --oneline 2>/dev/null)" ]; then
            echo "Manually copying files..."
            cp -r "${BASE_DIR}/${source_path}"/* .
            cp -r "${BASE_DIR}/${source_path}"/.[!.]* . 2>/dev/null || true
            git add .
            git commit -m "Initial commit: Extract ${source_path} from monorepo"
        fi
        
        echo -e "${YELLOW}⚠ History may be squashed. Install git-filter-repo for full history.${NC}"
    fi
    
    # Show structure
    echo -e "\n${GREEN}Repository contents:${NC}"
    ls -la | head -15
    
    # Check GitHub and offer to push
    echo -e "\n${YELLOW}Checking GitHub repository...${NC}"
    if git ls-remote --exit-code "https://github.com/${ORG}/${repo_name}.git" &>/dev/null; then
        echo -e "${GREEN}✓ Repository ${ORG}/${repo_name} exists${NC}"
        read -p "Push to GitHub? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git remote add origin "https://github.com/${ORG}/${repo_name}.git" 2>/dev/null || \
            git remote set-url origin "https://github.com/${ORG}/${repo_name}.git"
            
            BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null || echo "main")
            echo "Pushing to ${BRANCH}..."
            git push -u origin "${BRANCH}" --force
            echo -e "${GREEN}✓ Pushed to GitHub${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Repository ${ORG}/${repo_name} does not exist on GitHub${NC}"
        echo "Create it first, then:"
        echo "  cd ${temp_repo}"
        echo "  git remote add origin https://github.com/${ORG}/${repo_name}.git"
        BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null || echo "main")
        echo "  git push -u origin ${BRANCH}"
    fi
    
    echo -e "${GREEN}✓ ${repo_name} ready in: ${temp_repo}${NC}\n"
    cd "${BASE_DIR}"
}

main() {
    echo -e "${GREEN}=== Simple Repository Splitter ===${NC}"
    echo "This script extracts single-directory repositories"
    echo ""
    
    # Ask for pages name
    read -p "Name for pages repo? (web/pages) [default: web]: " pages_name
    pages_name=${pages_name:-web}
    
    mkdir -p "${TEMP_DIR}"
    
    case "${REPO_TO_SPLIT}" in
        web|pages)
            extract_repo "${pages_name}" "pages"
            ;;
        homeassistant)
            extract_repo "homeassistant" "homeassistant"
            ;;
        both|*)
            extract_repo "${pages_name}" "pages"
            extract_repo "homeassistant" "homeassistant"
            ;;
    esac
    
    echo -e "${GREEN}=== Done ===${NC}"
    echo -e "Temporary repos are in: ${TEMP_DIR}"
    echo "Review them before cleaning up!"
}

main

