#!/bin/bash

# Script to extract web and homeassistant repos from the firmware monorepo
# This script will:
# 1. Extract pages/ to brewos-io/web
# 2. Extract homeassistant/ to brewos-io/homeassistant
# 3. Set up proper files (README, LICENSE, .gitignore, workflows) for each
# 4. Optionally clean up the firmware repo

set -e

ORG="brewos-io"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP_DIR="${BASE_DIR}/.extract-temp"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== BrewOS Repository Extraction ===${NC}\n"

# Check if we're in a git repo
if [ ! -d "${BASE_DIR}/.git" ]; then
    echo -e "${RED}Error: Not a git repository${NC}"
    exit 1
fi

# Function to extract web repo
extract_web() {
    echo -e "\n${GREEN}=== Extracting Web Repository ===${NC}"
    
    if [ ! -d "${BASE_DIR}/pages" ]; then
        echo -e "${RED}Error: pages/ directory not found${NC}"
        return 1
    fi
    
    local web_dir="${TEMP_DIR}/web"
    mkdir -p "${web_dir}"
    
    # Clone the repo to extract with history
    echo "Cloning repository for history extraction..."
    git clone "${BASE_DIR}" "${web_dir}" --no-hardlinks
    cd "${web_dir}"
    
    # Check for git-filter-repo
    if command -v git-filter-repo &> /dev/null; then
        echo "Using git-filter-repo to extract pages/ with full history..."
        git filter-repo --path pages --path-rename pages:. --force
    else
        echo -e "${YELLOW}git-filter-repo not found, using manual extraction${NC}"
        echo "Installing git-filter-repo is recommended for better history preservation"
        
        # Manual extraction - keep only pages directory
        git checkout -b main 2>/dev/null || git checkout main
        
        # Remove everything except pages
        git rm -rf --ignore-unmatch \
            src homeassistant scripts docs assets \
            .github/workflows/build_firmware.yml \
            .github/workflows/build_web.yml \
            .github/workflows/ci.yml \
            .github/workflows/deploy.yml \
            .github/workflows/dev-release.yml \
            .github/workflows/release.yml \
            README.md SETUP.md CONTRIBUTING.md CODE_OF_CONDUCT.md TESTERS.md \
            VERSION version.json cliff.toml \
            2>/dev/null || true
        
        # Move pages to root
        if [ -d "pages" ]; then
            git mv pages/* . 2>/dev/null || mv pages/* .
            git mv pages/.[!.]* . 2>/dev/null || true
            rmdir pages 2>/dev/null || true
        fi
        
        git add -A
        git commit -m "Extract pages directory from monorepo" || true
    fi
    
    # Copy LICENSE
    if [ -f "${BASE_DIR}/LICENSE" ]; then
        cp "${BASE_DIR}/LICENSE" .
        git add LICENSE
        git commit -m "Add LICENSE" || true
    fi
    
    # Create .gitignore for web
    cat > .gitignore << 'EOF'
# Dependencies
node_modules/
.pnp
.pnp.js

# Build output
dist/
.output/
.astro/

# Environment variables
.env
.env.local
.env.production.local

# Debug
npm-debug.log*
yarn-debug.log*
yarn-error.log*

# OS
.DS_Store
Thumbs.db

# IDE
.vscode/
.idea/
*.swp
*.swo

# Astro
.astro/
EOF
    git add .gitignore
    git commit -m "Add .gitignore" || true
    
    # Copy assets directory if it exists in the original repo
    if [ -d "${BASE_DIR}/assets" ]; then
        echo "Copying assets directory..."
        cp -r "${BASE_DIR}/assets" .
        git add assets/
        git commit -m "Add assets directory" || true
    fi
    
    # Create GitHub workflow for Pages
    mkdir -p .github/workflows
    cat > .github/workflows/pages.yml << 'EOF'
name: Deploy to GitHub Pages

on:
  push:
    branches: ["main"]
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: pages
  cancel-in-progress: true

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v6

      - name: Setup Node.js
        uses: actions/setup-node@v6
        with:
          node-version: "20"
          cache: "npm"

      - name: Install dependencies
        run: npm ci

      - name: Copy assets to public folder
        run: |
          if [ -d "assets" ]; then
            rm -f public/assets
            cp -r assets public/assets
          fi

      - name: Build Astro site
        run: npm run build

      - name: Upload artifact
        uses: actions/upload-pages-artifact@v4
        with:
          path: dist

  deploy:
    needs: build
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-latest
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
EOF
    git add .github/workflows/pages.yml
    git commit -m "Add GitHub Pages workflow" || true
    
    # Update GitHub links from old repo to new org
    echo "Updating GitHub repository links..."
    find . -type f \( -name "*.astro" -o -name "*.md" -o -name "*.ts" -o -name "*.tsx" -o -name "*.js" -o -name "*.jsx" \) \
        -not -path "./node_modules/*" \
        -not -path "./.git/*" \
        -exec sed -i.bak \
            -e 's|github.com/mizrachiran/brewos|github.com/brewos-io/firmware|g' \
            -e 's|github.com/brewos-io/all|github.com/brewos-io/firmware|g' \
            {} \; 2>/dev/null || true
    find . -name "*.bak" -delete 2>/dev/null || true
    git add -A
    git commit -m "Update GitHub repository links to brewos-io/firmware" || true
    
    # Create or update README
    if [ ! -f "README.md" ]; then
        cat > README.md << 'EOF'
# BrewOS Website

Marketing and documentation website for the BrewOS project. Built with [Astro](https://astro.build/) and deployed to GitHub Pages.

## Development

```bash
npm install
npm run dev
```

The site will be available at `http://localhost:4321`

## Build

```bash
npm run build
```

Build output will be in the `dist/` directory.

## Deployment

The site is automatically deployed to GitHub Pages on pushes to `main` branch via GitHub Actions.

## Project Structure

```
.
├── src/
│   ├── components/     # Astro components
│   ├── layouts/        # Page layouts
│   ├── pages/          # Route pages
│   └── styles/         # Global styles
├── public/             # Static assets (copied during build)
├── assets/             # Source assets (logos, images)
└── astro.config.mjs    # Astro configuration
```

## Related Repositories

- [Firmware](https://github.com/brewos-io/firmware) - Main firmware repository (ESP32, Pico, web UI, cloud)
- [Home Assistant Integration](https://github.com/brewos-io/homeassistant) - HA custom component and Lovelace card

## License

Licensed under the Apache License 2.0 with Commons Clause - see [LICENSE](LICENSE) file for details.
EOF
        git add README.md
        git commit -m "Add README" || true
    fi
    
    # Fix remote to point to GitHub (not local repo)
    echo "Setting up GitHub remote..."
    git remote remove origin 2>/dev/null || true
    git remote add origin "https://github.com/${ORG}/web.git" 2>/dev/null || \
    git remote set-url origin "https://github.com/${ORG}/web.git"
    
    echo -e "${GREEN}✓ Web repository prepared${NC}"
    echo -e "  Location: ${web_dir}"
    echo -e "  Remote: https://github.com/${ORG}/web.git"
    echo -e "  Next: cd ${web_dir} && git push -u origin main --force"
    
    cd "${BASE_DIR}"
}

# Function to extract homeassistant repo
extract_homeassistant() {
    echo -e "\n${GREEN}=== Extracting Home Assistant Repository ===${NC}"
    
    if [ ! -d "${BASE_DIR}/homeassistant" ]; then
        echo -e "${RED}Error: homeassistant/ directory not found${NC}"
        return 1
    fi
    
    local ha_dir="${TEMP_DIR}/homeassistant"
    mkdir -p "${ha_dir}"
    
    # Clone the repo to extract with history
    echo "Cloning repository for history extraction..."
    git clone "${BASE_DIR}" "${ha_dir}" --no-hardlinks
    cd "${ha_dir}"
    
    # Check for git-filter-repo
    if command -v git-filter-repo &> /dev/null; then
        echo "Using git-filter-repo to extract homeassistant/ with full history..."
        git filter-repo --path homeassistant --path-rename homeassistant:. --force
    else
        echo -e "${YELLOW}git-filter-repo not found, using manual extraction${NC}"
        
        # Manual extraction
        git checkout -b main 2>/dev/null || git checkout main
        
        # Remove everything except homeassistant
        git rm -rf --ignore-unmatch \
            src pages scripts docs assets \
            .github/workflows/build_firmware.yml \
            .github/workflows/build_web.yml \
            .github/workflows/ci.yml \
            .github/workflows/deploy.yml \
            .github/workflows/dev-release.yml \
            .github/workflows/release.yml \
            .github/workflows/pages.yml \
            README.md SETUP.md CONTRIBUTING.md CODE_OF_CONDUCT.md TESTERS.md \
            VERSION version.json cliff.toml \
            2>/dev/null || true
        
        # Move homeassistant to root
        if [ -d "homeassistant" ]; then
            git mv homeassistant/* . 2>/dev/null || mv homeassistant/* .
            git mv homeassistant/.[!.]* . 2>/dev/null || true
            rmdir homeassistant 2>/dev/null || true
        fi
        
        git add -A
        git commit -m "Extract homeassistant directory from monorepo" || true
    fi
    
    # Copy LICENSE
    if [ -f "${BASE_DIR}/LICENSE" ]; then
        cp "${BASE_DIR}/LICENSE" .
        git add LICENSE
        git commit -m "Add LICENSE" || true
    fi
    
    # Create .gitignore for homeassistant
    cat > .gitignore << 'EOF'
# Python
__pycache__/
*.py[cod]
*$py.class
*.so
.Python
env/
venv/
ENV/
.venv

# IDE
.vscode/
.idea/
*.swp
*.swo

# OS
.DS_Store
Thumbs.db

# Home Assistant
.homeassistant/
EOF
    git add .gitignore
    git commit -m "Add .gitignore" || true
    
    # Update README if it exists (it should)
    if [ -f "README.md" ]; then
        # Add links to related repos at the top
        if ! grep -q "Related Repositories" README.md; then
            cat > README_TEMP.md << 'EOF'
# BrewOS Home Assistant Integration

## Related Repositories

- [Firmware](https://github.com/brewos-io/firmware) - Main firmware repository
- [Web Site](https://github.com/brewos-io/web) - Project website

---

EOF
            cat README_TEMP.md README.md > README_NEW.md
            mv README_NEW.md README.md
            rm README_TEMP.md
            git add README.md
            git commit -m "Update README with related repositories" || true
        fi
    else
        # Create basic README
        cat > README.md << 'EOF'
# BrewOS Home Assistant Integration

Home Assistant integration for BrewOS espresso machines.

## Related Repositories

- [Firmware](https://github.com/brewos-io/firmware) - Main firmware repository
- [Web Site](https://github.com/brewos-io/web) - Project website

See the [README in the homeassistant directory](https://github.com/brewos-io/homeassistant) for installation and usage instructions.
EOF
        git add README.md
        git commit -m "Add README" || true
    fi
    
    # Fix remote to point to GitHub (not local repo)
    echo "Setting up GitHub remote..."
    git remote remove origin 2>/dev/null || true
    git remote add origin "https://github.com/${ORG}/homeassistant.git" 2>/dev/null || \
    git remote set-url origin "https://github.com/${ORG}/homeassistant.git"
    
    echo -e "${GREEN}✓ Home Assistant repository prepared${NC}"
    echo -e "  Location: ${ha_dir}"
    echo -e "  Remote: https://github.com/${ORG}/homeassistant.git"
    echo -e "  Next: cd ${ha_dir} && git push -u origin main --force"
    
    cd "${BASE_DIR}"
}

# Main execution
main() {
    mkdir -p "${TEMP_DIR}"
    
    echo "This script will extract:"
    echo "  1. pages/ → brewos-io/web"
    echo "  2. homeassistant/ → brewos-io/homeassistant"
    echo ""
    echo "Each repository will be prepared with:"
    echo "  - Full git history (if git-filter-repo is available)"
    echo "  - LICENSE file"
    echo "  - README.md"
    echo "  - .gitignore"
    echo "  - GitHub workflows (for web)"
    echo ""
    read -p "Continue? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
    
    extract_web
    extract_homeassistant
    
    echo -e "\n${GREEN}=== Extraction Complete ===${NC}"
    echo -e "\n${YELLOW}Next steps:${NC}"
    echo ""
    echo "1. Review the extracted repositories in: ${TEMP_DIR}"
    echo ""
    echo "2. Push web repository:"
    echo "   cd ${TEMP_DIR}/web"
    echo "   git push -u origin main --force"
    echo ""
    echo "3. Push homeassistant repository:"
    echo "   cd ${TEMP_DIR}/homeassistant"
    echo "   git push -u origin main --force"
    echo ""
    echo "Note: GitHub remotes are already configured!"
    echo ""
    echo "4. After pushing, clean up firmware repo (run cleanup script)"
    echo ""
    read -p "Open extraction directories? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        open "${TEMP_DIR}/web" 2>/dev/null || echo "cd ${TEMP_DIR}/web"
        open "${TEMP_DIR}/homeassistant" 2>/dev/null || echo "cd ${TEMP_DIR}/homeassistant"
    fi
}

main

