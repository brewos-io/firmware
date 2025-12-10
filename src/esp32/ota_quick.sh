#!/bin/bash
# Quick OTA upload (no build) - use when firmware is already built
# Usage: ./ota_quick.sh [IP_ADDRESS]

ESP32_IP="${1:-192.168.1.100}"
FIRMWARE=".pio/build/esp32s3/firmware.bin"

cd "$(dirname "$0")" 2>/dev/null || true

if [ ! -f "$FIRMWARE" ]; then
    echo "Firmware not found. Run ./ota_update.sh to build first."
    exit 1
fi

echo "Uploading to $ESP32_IP..."
curl -X POST "http://$ESP32_IP/api/ota/upload" -F "firmware=@$FIRMWARE" --progress-bar && echo -e "\n✓ Done! Rebooting..."

