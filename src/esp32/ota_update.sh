#!/bin/bash
# BrewOS ESP32 Local Development OTA Update Script
# Usage: ./ota_update.sh [IP_ADDRESS] [--firmware-only] [--no-build-app]
# Example: ./ota_update.sh 192.168.1.100
#          ./ota_update.sh brewos.local
#          ./ota_update.sh --firmware-only
#
# This script is for LOCAL DEVELOPMENT - uploads locally built firmware AND web files
# For production updates, use the webapp which downloads from GitHub releases
#
# Options:
#   --firmware-only   Only upload firmware, skip web app build and upload
#   --no-build-app    Skip web app build (use existing data/ files)

# Default IP (change this to your ESP32's IP or use brewos.local)
DEFAULT_IP="brewos.local"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Parse arguments
FIRMWARE_ONLY=false
NO_BUILD_APP=false
ESP32_IP=""

for arg in "$@"; do
    case "$arg" in
        --firmware-only)
            FIRMWARE_ONLY=true
            ;;
        --no-build-app)
            NO_BUILD_APP=true
            ;;
        -*)
            echo -e "${RED}Unknown option: $arg${NC}"
            echo "Usage: $0 [IP_ADDRESS] [--firmware-only] [--no-build-app]"
            exit 1
            ;;
        *)
            if [ -z "$ESP32_IP" ]; then
                ESP32_IP="$arg"
            fi
            ;;
    esac
done

# Use default IP if not provided
ESP32_IP="${ESP32_IP:-$DEFAULT_IP}"

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  BrewOS ESP32 Local Dev OTA Update   ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Note: This is for LOCAL DEVELOPMENT only${NC}"
echo -e "${YELLOW}Production updates use GitHub releases via webapp${NC}"
if [ "$FIRMWARE_ONLY" = true ]; then
    echo -e "${CYAN}Mode: Firmware only (skipping web files)${NC}"
else
    echo -e "${CYAN}Mode: Full update (firmware + web files)${NC}"
fi
echo ""

# Check if we're in the right directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ ! -f "$SCRIPT_DIR/platformio.ini" ]; then
    echo -e "${RED}Error: platformio.ini not found. Run from esp32 directory.${NC}"
    exit 1
fi
cd "$SCRIPT_DIR"

FIRMWARE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_APP_SCRIPT="$SCRIPT_DIR/../scripts/build_app_for_esp32.sh"
WEB_DATA_DIR="$SCRIPT_DIR/data"
FIRMWARE=".pio/build/esp32s3/firmware.bin"
LITTLEFS=".pio/build/esp32s3/littlefs.bin"

# Determine total steps
if [ "$FIRMWARE_ONLY" = true ]; then
    TOTAL_STEPS=3
else
    TOTAL_STEPS=6
fi
STEP=0

# ============================================================
# Step: Build web app (unless --firmware-only or --no-build-app)
# ============================================================
if [ "$FIRMWARE_ONLY" = false ] && [ "$NO_BUILD_APP" = false ]; then
    STEP=$((STEP + 1))
    echo -e "${YELLOW}[$STEP/$TOTAL_STEPS] Building web app...${NC}"
    
    if [ ! -f "$BUILD_APP_SCRIPT" ]; then
        echo -e "${RED}Error: build_app_for_esp32.sh not found at $BUILD_APP_SCRIPT${NC}"
        echo -e "${YELLOW}Use --no-build-app to skip web app build and use existing data/ files${NC}"
        exit 1
    fi
    
    # Clear and rebuild data directory
    if [ -d "$WEB_DATA_DIR" ]; then
        rm -rf "$WEB_DATA_DIR"
    fi
    mkdir -p "$WEB_DATA_DIR"
    
    if ! "$BUILD_APP_SCRIPT" --build-only; then
        echo -e "${RED}Web app build failed!${NC}"
        exit 1
    fi
    
    # Verify web files were built
    if [ ! -f "$WEB_DATA_DIR/index.html" ]; then
        echo -e "${RED}Error: Web app build produced no index.html${NC}"
        exit 1
    fi
    
    WEB_FILE_COUNT=$(find "$WEB_DATA_DIR" -type f | wc -l | tr -d ' ')
    WEB_DATA_SIZE=$(du -sh "$WEB_DATA_DIR" | cut -f1)
    echo -e "${GREEN}Web app built: $WEB_FILE_COUNT files ($WEB_DATA_SIZE)${NC}"
    echo ""
elif [ "$FIRMWARE_ONLY" = false ] && [ "$NO_BUILD_APP" = true ]; then
    # Using existing data/ files - verify they exist
    if [ ! -d "$WEB_DATA_DIR" ] || [ ! -f "$WEB_DATA_DIR/index.html" ]; then
        echo -e "${RED}Error: No web files found in $WEB_DATA_DIR${NC}"
        echo -e "${YELLOW}Run without --no-build-app to build the web app first${NC}"
        exit 1
    fi
    WEB_FILE_COUNT=$(find "$WEB_DATA_DIR" -type f | wc -l | tr -d ' ')
    WEB_DATA_SIZE=$(du -sh "$WEB_DATA_DIR" | cut -f1)
    echo -e "${BLUE}Using existing web files: $WEB_FILE_COUNT files ($WEB_DATA_SIZE)${NC}"
    echo ""
fi

# ============================================================
# Step: Build firmware
# ============================================================
STEP=$((STEP + 1))
echo -e "${YELLOW}[$STEP/$TOTAL_STEPS] Building firmware...${NC}"
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
FIRMWARE_SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
echo -e "${BLUE}Firmware size: $FIRMWARE_SIZE${NC}"
echo ""

# ============================================================
# Step: Check connectivity
# ============================================================
STEP=$((STEP + 1))
echo -e "${YELLOW}[$STEP/$TOTAL_STEPS] Checking ESP32 at $ESP32_IP...${NC}"
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

# ============================================================
# Step: Upload firmware via OTA
# ============================================================
STEP=$((STEP + 1))
echo -e "${YELLOW}[$STEP/$TOTAL_STEPS] Uploading firmware via OTA...${NC}"
echo -e "${YELLOW}      (ESP32 will reboot automatically after flash)${NC}"
echo ""

# Use the direct ESP32 OTA endpoint that flashes immediately
BODY_FILE=$(mktemp)
HTTP_CODE=$(curl -X POST "http://$ESP32_IP/api/ota/esp32/upload" \
    -F "firmware=@$FIRMWARE" \
    --progress-bar \
    --max-time 120 \
    -o "$BODY_FILE" \
    -w "%{http_code}" 2>&1)
CURL_EXIT=$?
BODY=$(cat "$BODY_FILE" 2>/dev/null)
rm -f "$BODY_FILE"

# Extract just the HTTP code (last 3 chars) in case progress bar leaked into output
HTTP_CODE=$(echo "$HTTP_CODE" | grep -oE '[0-9]+$')

echo ""
if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}✓ Firmware uploaded successfully!${NC}"
elif [ "$HTTP_CODE" = "000" ] || [ "$HTTP_CODE" = "100" ] || [ "$CURL_EXIT" = "56" ] || [ "$CURL_EXIT" = "52" ]; then
    # Connection closed/reset - expected when ESP32 reboots after successful flash
    echo -e "${GREEN}✓ Firmware uploaded (ESP32 rebooted before confirming - this is normal)${NC}"
else
    echo -e "${RED}Firmware OTA Upload Failed!${NC}"
    echo "HTTP Code: $HTTP_CODE"
    echo "curl exit code: $CURL_EXIT"
    if [ -n "$BODY" ]; then
        echo "Response: $BODY"
    fi
    exit 1
fi
echo ""

# ============================================================
# If firmware-only, we're done
# ============================================================
if [ "$FIRMWARE_ONLY" = true ]; then
    echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ✓ ESP32 OTA Update Successful!      ║${NC}"
    echo -e "${GREEN}║    (firmware only)                   ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BLUE}ESP32 is rebooting with new firmware...${NC}"
    echo -e "${BLUE}Wait ~5 seconds, then access: http://$ESP32_IP${NC}"
    exit 0
fi

# ============================================================
# Step: Wait for ESP32 to reboot
# ============================================================
STEP=$((STEP + 1))
echo -e "${YELLOW}[$STEP/$TOTAL_STEPS] Waiting for ESP32 to reboot...${NC}"

# Wait for ESP32 to come back up
MAX_WAIT=30
WAITED=0
REBOOT_WAIT=3

# Initial wait to let ESP32 shut down
echo -e "${BLUE}Waiting ${REBOOT_WAIT}s for ESP32 to restart...${NC}"
sleep $REBOOT_WAIT
WAITED=$REBOOT_WAIT

# Poll until ESP32 responds
while [ $WAITED -lt $MAX_WAIT ]; do
    if curl -s --connect-timeout 2 "http://$ESP32_IP/test" > /dev/null 2>&1; then
        echo -e "${GREEN}ESP32 is back online! (took ${WAITED}s)${NC}"
        echo ""
        break
    fi
    sleep 1
    WAITED=$((WAITED + 1))
    printf "."
done

if [ $WAITED -ge $MAX_WAIT ]; then
    echo ""
    echo -e "${RED}ESP32 did not come back online within ${MAX_WAIT}s${NC}"
    echo -e "${YELLOW}The firmware was flashed successfully, but web files could not be uploaded.${NC}"
    echo -e "${YELLOW}You can try again with: $0 $ESP32_IP --no-build-app${NC}"
    exit 1
fi

# Brief extra delay to let web server fully initialize
sleep 1

# ============================================================
# Step: Upload web files via OTA
# ============================================================
STEP=$((STEP + 1))
echo -e "${YELLOW}[$STEP/$TOTAL_STEPS] Uploading web files to ESP32...${NC}"

# Step 6a: Start web OTA (cleans old assets)
echo -e "${BLUE}Cleaning old web assets on ESP32...${NC}"
START_RESPONSE=$(curl -s -X POST "http://$ESP32_IP/api/ota/web/start" --max-time 10)
START_EXIT=$?

if [ $START_EXIT -ne 0 ]; then
    echo -e "${RED}Failed to start web OTA (curl exit: $START_EXIT)${NC}"
    echo -e "${YELLOW}Firmware was updated. Web files can be uploaded later.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Old assets cleaned${NC}"

# Step 6b: Upload each file
UPLOAD_COUNT=0
UPLOAD_ERRORS=0

# Upload files from data directory, preserving relative paths
while IFS= read -r -d '' filepath; do
    # Get relative path from data directory (e.g., "index.html", "assets/main-abc123.js")
    REL_PATH="${filepath#$WEB_DATA_DIR/}"
    
    # Upload the file
    HTTP_CODE=$(curl -s -X POST "http://$ESP32_IP/api/ota/web/upload" \
        -F "file=@${filepath};filename=${REL_PATH}" \
        --max-time 30 \
        -o /dev/null \
        -w "%{http_code}")
    
    if [ "$HTTP_CODE" = "200" ]; then
        UPLOAD_COUNT=$((UPLOAD_COUNT + 1))
        FILE_SIZE=$(ls -lh "$filepath" | awk '{print $5}')
        echo -e "  ${GREEN}✓${NC} ${REL_PATH} (${FILE_SIZE})"
    else
        UPLOAD_ERRORS=$((UPLOAD_ERRORS + 1))
        echo -e "  ${RED}✗${NC} ${REL_PATH} (HTTP $HTTP_CODE)"
    fi
done < <(find "$WEB_DATA_DIR" -type f -print0)

echo ""
echo -e "${BLUE}Uploaded: $UPLOAD_COUNT files, Errors: $UPLOAD_ERRORS${NC}"

# Step 6c: Complete web OTA
COMPLETE_RESPONSE=$(curl -s -X POST "http://$ESP32_IP/api/ota/web/complete" --max-time 10)
echo -e "${BLUE}Server response: $COMPLETE_RESPONSE${NC}"

# ============================================================
# Done!
# ============================================================
echo ""
if [ $UPLOAD_ERRORS -eq 0 ]; then
    echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ✓ Full OTA Update Successful!       ║${NC}"
    echo -e "${GREEN}║    Firmware + Web Files updated       ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
else
    echo -e "${YELLOW}╔══════════════════════════════════════╗${NC}"
    echo -e "${YELLOW}║  ⚠ OTA Update Partially Successful   ║${NC}"
    echo -e "${YELLOW}║    Firmware OK, $UPLOAD_ERRORS web file(s) failed  ║${NC}"
    echo -e "${YELLOW}╚══════════════════════════════════════╝${NC}"
fi
echo ""
echo -e "${BLUE}Access your ESP32 at: http://$ESP32_IP${NC}"
