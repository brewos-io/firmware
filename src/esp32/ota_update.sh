#!/bin/bash
# BrewOS ESP32 OTA Update Script
# Usage: ./ota_update.sh [IP_ADDRESS]
# Example: ./ota_update.sh 192.168.1.100

# Default IP (change this to your ESP32's IP)
DEFAULT_IP="192.168.1.100"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get IP from argument or use default
ESP32_IP="${1:-$DEFAULT_IP}"

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     BrewOS ESP32 OTA Updater         ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    cd "$(dirname "$0")" 2>/dev/null || true
    if [ ! -f "platformio.ini" ]; then
        echo -e "${RED}Error: platformio.ini not found. Run from esp32 directory.${NC}"
        exit 1
    fi
fi

FIRMWARE=".pio/build/esp32s3/firmware.bin"

# Step 1: Build
echo -e "${YELLOW}[1/3] Building firmware...${NC}"
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

# Show firmware size
SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
echo -e "${BLUE}Firmware size: $SIZE${NC}"
echo ""

# Step 2: Check connectivity
echo -e "${YELLOW}[2/3] Checking ESP32 at $ESP32_IP...${NC}"
if ! curl -s --connect-timeout 3 "http://$ESP32_IP/test" > /dev/null; then
    echo -e "${RED}Error: Cannot reach ESP32 at $ESP32_IP${NC}"
    echo -e "${YELLOW}Make sure:${NC}"
    echo "  - ESP32 is powered on"
    echo "  - You're on the same network"
    echo "  - IP address is correct"
    echo ""
    echo "Usage: $0 <ESP32_IP>"
    exit 1
fi
echo -e "${GREEN}ESP32 is reachable!${NC}"
echo ""

# Step 3: Upload
echo -e "${YELLOW}[3/3] Uploading firmware via OTA...${NC}"
echo ""

RESPONSE=$(curl -X POST "http://$ESP32_IP/api/ota/upload" \
    -F "firmware=@$FIRMWARE" \
    --progress-bar \
    -w "\n%{http_code}" 2>&1)

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')

echo ""
if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║     OTA Update Successful! ✓         ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BLUE}ESP32 is rebooting...${NC}"
    echo "Wait a few seconds, then access: http://$ESP32_IP"
else
    echo -e "${RED}OTA Update Failed!${NC}"
    echo "HTTP Code: $HTTP_CODE"
    echo "Response: $BODY"
    exit 1
fi

