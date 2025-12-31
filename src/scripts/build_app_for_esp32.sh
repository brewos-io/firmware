#!/bin/bash
# Build app from external app repository for ESP32 deployment
# 
# This script builds the app from the sibling app repository and syncs it to ESP32 data folder
#
# Usage:
#   ./scripts/build_app_for_esp32.sh          # Build and sync
#   ./scripts/build_app_for_esp32.sh --build-only  # Only build, don't sync

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
APP_DIR="$FIRMWARE_DIR/../app"
ESP32_DATA="$SCRIPT_DIR/../esp32/data"

# Colors
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}🔨 Building app for ESP32 from external repository...${NC}"

# Check if app directory exists
if [ ! -d "$APP_DIR" ]; then
  echo -e "${RED}❌ App repository not found at $APP_DIR${NC}"
  echo "Please ensure the app repository is cloned as a sibling to the firmware repository"
  exit 1
fi

# Check if app has package.json
if [ ! -f "$APP_DIR/package.json" ]; then
  echo -e "${RED}❌ App repository appears to be empty or not initialized${NC}"
  exit 1
fi

cd "$APP_DIR"

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
  echo -e "${YELLOW}📥 Installing app dependencies...${NC}"
  npm install
fi

# Build for ESP32
echo -e "${BLUE}🏗️  Building app for ESP32...${NC}"
ESP32_DATA_DIR="$ESP32_DATA" npm run build:esp32

if [ "$1" = "--build-only" ]; then
  echo -e "${GREEN}✅ Build complete!${NC}"
  exit 0
fi

echo -e "${GREEN}✅ Build complete!${NC}"
echo ""
echo -e "${BLUE}📊 ESP32 data folder contents:${NC}"
du -sh "$ESP32_DATA"
echo ""
echo "Next steps:"
echo "  cd src/esp32 && pio run -e esp32s3 -t uploadfs"

