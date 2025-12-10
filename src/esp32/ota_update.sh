#!/bin/bash
# BrewOS ESP32 Local Development OTA Update Script
# Usage: ./ota_update.sh [IP_ADDRESS]
# Example: ./ota_update.sh 192.168.1.100
#          ./ota_update.sh brewos.local
#
# This script is for LOCAL DEVELOPMENT - uploads locally built firmware
# For production updates, use the webapp which downloads from GitHub releases
# (includes both firmware and filesystem updates)

# Default IP (change this to your ESP32's IP or use brewos.local)
DEFAULT_IP="brewos.local"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get IP from argument or use default
ESP32_IP="${1:-$DEFAULT_IP}"

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  BrewOS ESP32 Local Dev OTA Update   ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Note: This is for LOCAL DEVELOPMENT only${NC}"
echo -e "${YELLOW}Production updates use GitHub releases via webapp${NC}"
echo ""

# Check if we're in the right directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ ! -f "$SCRIPT_DIR/platformio.ini" ]; then
    echo -e "${RED}Error: platformio.ini not found. Run from esp32 directory.${NC}"
    exit 1
fi
cd "$SCRIPT_DIR"

FIRMWARE=".pio/build/esp32s3/firmware.bin"
LITTLEFS=".pio/build/esp32s3/littlefs.bin"

# Step 1: Build
echo -e "${YELLOW}[1/4] Building firmware...${NC}"
if ! pio run -e esp32s3; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
echo -e "${GREEN}Build successful!${NC}"
echo ""

# Check firmware exists
if [ ! -f "$FIRMWARE" ]; then
    echo -e "${RED}Error: Firmware not found at $FIRMWARE${NC}"
    exit 1
fi

# Step 2: Build filesystem
echo -e "${YELLOW}[2/4] Building filesystem image...${NC}"
if ! pio run -e esp32s3 -t buildfs; then
    echo -e "${YELLOW}Warning: Filesystem build failed - continuing with firmware only${NC}"
    LITTLEFS=""
else
    if [ ! -f "$LITTLEFS" ]; then
        echo -e "${YELLOW}Warning: LittleFS image not found - continuing with firmware only${NC}"
        LITTLEFS=""
    fi
fi
echo ""

# Show sizes
FIRMWARE_SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
echo -e "${BLUE}Firmware size: $FIRMWARE_SIZE${NC}"
if [ -n "$LITTLEFS" ]; then
    LITTLEFS_SIZE=$(ls -lh "$LITTLEFS" | awk '{print $5}')
    echo -e "${BLUE}Filesystem size: $LITTLEFS_SIZE${NC}"
fi
echo ""

# Step 3: Check connectivity
echo -e "${YELLOW}[3/4] Checking ESP32 at $ESP32_IP...${NC}"
if ! curl -s --connect-timeout 5 "http://$ESP32_IP/test" > /dev/null; then
    echo -e "${RED}Error: Cannot reach ESP32 at $ESP32_IP${NC}"
    echo -e "${YELLOW}Make sure:${NC}"
    echo "  - ESP32 is powered on"
    echo "  - You're on the same network"
    echo "  - IP address is correct (try the actual IP instead of brewos.local)"
    echo ""
    echo "Usage: $0 <ESP32_IP>"
    exit 1
fi
echo -e "${GREEN}ESP32 is reachable!${NC}"
echo ""

# Step 4: Upload firmware
echo -e "${YELLOW}[4/4] Uploading firmware via OTA...${NC}"
echo ""

RESPONSE=$(curl -X POST "http://$ESP32_IP/api/ota/upload" \
    -F "firmware=@$FIRMWARE" \
    --progress-bar \
    -w "\n%{http_code}" 2>&1)

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')

echo ""
if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}✓ Firmware uploaded!${NC}"
    
    # Note: The uploaded firmware is saved to LittleFS but NOT automatically flashed
    # For local dev, you need to manually trigger the flash via webapp or use GitHub OTA
    echo ""
    echo -e "${YELLOW}⚠ Note: Firmware is uploaded but not yet flashed.${NC}"
    echo -e "${YELLOW}   For local development, use the webapp to trigger OTA,${NC}"
    echo -e "${YELLOW}   or use GitHub OTA which handles both firmware and filesystem.${NC}"
    echo ""
    echo -e "${BLUE}Access your device at: http://$ESP32_IP${NC}"
else
    echo -e "${RED}OTA Upload Failed!${NC}"
    echo "HTTP Code: $HTTP_CODE"
    echo "Response: $BODY"
    exit 1
fi
