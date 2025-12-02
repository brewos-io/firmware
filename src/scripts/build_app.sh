#!/bin/bash
# Build BrewOS Web UI for ESP32 and Cloud

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
WEB_DIR="$PROJECT_ROOT/src/web"
ESP32_DATA_DIR="$PROJECT_ROOT/src/esp32/data"

echo "🔧 BrewOS Web UI Build Script"
echo "============================="

# Check if node_modules exists
if [ ! -d "$WEB_DIR/node_modules" ]; then
    echo "📦 Installing dependencies..."
    cd "$WEB_DIR"
    npm install
fi

cd "$WEB_DIR"

case "${1:-all}" in
    esp32)
        echo ""
        echo "📱 Building for ESP32..."
        npm run build:esp32
        echo "✅ ESP32 build complete: $ESP32_DATA_DIR"
        
        # Show size
        echo ""
        echo "📊 Bundle sizes:"
        du -sh "$ESP32_DATA_DIR"/* 2>/dev/null || echo "  (no files)"
        ;;
    
    cloud)
        echo ""
        echo "☁️  Building for Cloud..."
        npm run build
        echo "✅ Cloud build complete: $WEB_DIR/dist"
        ;;
    
    all)
        echo ""
        echo "📱 Building for ESP32..."
        npm run build:esp32
        echo "✅ ESP32 build complete"
        
        echo ""
        echo "☁️  Building for Cloud..."
        npm run build
        echo "✅ Cloud build complete"
        
        echo ""
        echo "📊 ESP32 bundle sizes:"
        du -sh "$ESP32_DATA_DIR"/* 2>/dev/null || echo "  (no files)"
        ;;
    
    *)
        echo "Usage: $0 [esp32|cloud|all]"
        exit 1
        ;;
esac

echo ""
echo "🎉 Build complete!"
