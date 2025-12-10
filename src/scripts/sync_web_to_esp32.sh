#!/bin/bash
# Sync web build output to ESP32 data folder
# Cleans old files and copies fresh build
#
# Usage:
#   ./scripts/sync_web_to_esp32.sh          # Sync existing build
#   ./scripts/sync_web_to_esp32.sh --build  # Build first, then sync

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEB_DIR="$SCRIPT_DIR/../web"
WEB_DIST="$WEB_DIR/dist"
ESP32_DATA="$SCRIPT_DIR/../esp32/data"

# Colors
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}🔄 Syncing web build to ESP32...${NC}"

# Check for --build flag
if [ "$1" = "--build" ]; then
    echo -e "${BLUE}📦 Building web app first...${NC}"
    cd "$WEB_DIR"
    
    # Install deps if needed
    if [ ! -d "node_modules" ]; then
        echo -e "${YELLOW}Installing dependencies...${NC}"
        npm install
    fi
    
    # Build outputs directly to esp32/data with emptyOutDir
    npm run build:esp32
    
    echo -e "${GREEN}✅ Build complete!${NC}"
    echo ""
    echo -e "${BLUE}📊 ESP32 data folder contents:${NC}"
    du -sh "$ESP32_DATA"
    ls -la "$ESP32_DATA"
    echo ""
    ls -la "$ESP32_DATA/assets" 2>/dev/null || true
    exit 0
fi

# Manual sync mode - sync from dist folder
if [ ! -d "$WEB_DIST" ]; then
    echo -e "${YELLOW}❌ Web dist not found at $WEB_DIST${NC}"
    echo "Run with --build to build first, or run 'npm run build' in src/web"
    exit 1
fi

# Clean ESP32 data folder
echo -e "${YELLOW}🧹 Cleaning old files from ESP32 data...${NC}"
rm -rf "$ESP32_DATA/assets" 2>/dev/null || true
rm -f "$ESP32_DATA/index.html" 2>/dev/null || true
rm -f "$ESP32_DATA/favicon.svg" 2>/dev/null || true
rm -f "$ESP32_DATA/logo.png" 2>/dev/null || true
rm -f "$ESP32_DATA/logo-icon.svg" 2>/dev/null || true
rm -f "$ESP32_DATA/manifest.json" 2>/dev/null || true
rm -f "$ESP32_DATA/sw.js" 2>/dev/null || true
rm -f "$ESP32_DATA/version-manifest.json" 2>/dev/null || true
rm -rf "$ESP32_DATA/.well-known" 2>/dev/null || true

# Copy fresh build
echo -e "${BLUE}📦 Copying new build...${NC}"
mkdir -p "$ESP32_DATA"
cp -r "$WEB_DIST"/* "$ESP32_DATA/"

# Remove files not needed on ESP32 (save space)
rm -rf "$ESP32_DATA/.well-known" 2>/dev/null || true

# Show what's being deployed
echo ""
echo -e "${BLUE}📊 ESP32 data folder contents:${NC}"
du -sh "$ESP32_DATA"
echo ""
ls -la "$ESP32_DATA"
echo ""
ls -la "$ESP32_DATA/assets" 2>/dev/null || true

echo ""
echo -e "${GREEN}✅ Sync complete!${NC}"
echo ""
echo "Next steps:"
echo "  cd src/esp32 && pio run -e esp32s3 -t uploadfs"
