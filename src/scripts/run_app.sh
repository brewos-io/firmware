#!/bin/bash
# Run BrewOS Web UI in development mode
# Usage: ./run_web.sh [--cloud]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEB_DIR="$SCRIPT_DIR/../web"

# Check if web directory exists
if [ ! -d "$WEB_DIR" ]; then
    echo "❌ Web directory not found: $WEB_DIR"
    exit 1
fi

cd "$WEB_DIR"

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
fi

# Check for --cloud flag
if [ "$1" == "--cloud" ]; then
    echo "☁️  Starting web app in cloud mode..."
    echo "   Make sure to set VITE_GOOGLE_CLIENT_ID in .env"
    npm run dev -- --mode cloud
else
    echo "🌐 Starting web app in local/ESP32 mode..."
    echo "   Connect to your ESP32 at http://brewos.local or its IP address"
    npm run dev
fi

