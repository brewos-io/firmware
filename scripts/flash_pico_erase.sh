#!/usr/bin/env bash
# Erase Pico flash completely, then prompt to flash the application UF2.
# Use this when the Pico boot loops or has corrupt persisted config (e.g. after
# config changes). Persisted config lives in the last
# flash sector; a normal UF2 copy does not overwrite it.
#
# Requires: picotool on PATH (from Pico SDK).
# The Pico must be connected and running (not in BOOTSEL). If it boot loops,
# see SETUP.md "Recovery: full flash erase" for the nuke-UF2 method.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$FIRMWARE_ROOT/src/pico/build"

if ! command -v picotool &>/dev/null; then
  echo "picotool not found. Install Pico SDK and add picotool to PATH."
  echo "See: https://github.com/raspberrypi/picotool"
  exit 1
fi

echo "Pico must be connected over USB and running (not in BOOTSEL)."
echo "If the Pico is in a boot loop, use the 'nuke' UF2 method in SETUP.md instead."
echo ""
read -r -p "Press Enter to erase Pico flash (or Ctrl+C to cancel)..."

picotool erase -a

echo ""
echo "Flash erased. Next:"
echo "  1. Put Pico in BOOTSEL mode (hold BOOTSEL, connect USB)."
echo "  2. Copy the application UF2 to the RPI-RP2 drive."
if [ -d "$BUILD_DIR" ]; then
  for f in "$BUILD_DIR"/brewos_*.uf2; do
    [ -f "$f" ] && echo "     Example: cp $(basename "$f") /Volumes/RPI-RP2/  # or your mount point"
    break
  done
fi
echo ""
