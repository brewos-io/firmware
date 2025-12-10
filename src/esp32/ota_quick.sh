#!/bin/bash
# Quick OTA upload (no build) - use when firmware is already built
# Usage: ./ota_quick.sh [IP_ADDRESS]
#
# Note: This uploads firmware to LittleFS but does NOT flash it.
# For local dev, use the webapp to trigger the actual flash, or use GitHub OTA.

ESP32_IP="${1:-brewos.local}"
FIRMWARE=".pio/build/esp32s3/firmware.bin"

cd "$(dirname "$0")" 2>/dev/null || true

if [ ! -f "$FIRMWARE" ]; then
    echo "Firmware not found. Run ./ota_update.sh to build first."
    exit 1
fi

echo "Uploading firmware to $ESP32_IP..."
echo "Note: This uploads to LittleFS. Use webapp or GitHub OTA to flash."

RESPONSE=$(curl -X POST "http://$ESP32_IP/api/ota/upload" \
    -F "firmware=@$FIRMWARE" \
    --progress-bar \
    -w "\n%{http_code}" 2>&1)

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)

echo ""
if [ "$HTTP_CODE" = "200" ]; then
    echo -e "\033[0;32m✓ Upload complete!\033[0m"
    echo "Note: Firmware uploaded but not flashed. Use webapp to trigger OTA."
else
    echo -e "\033[0;31m✗ Upload failed (HTTP $HTTP_CODE)\033[0m"
    exit 1
fi
