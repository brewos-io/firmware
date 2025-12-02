#!/bin/bash
# Run BrewOS Marketing Site in development mode
# Usage: ./run_site.sh [--build]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITE_DIR="$SCRIPT_DIR/../../pages"

# Check if pages directory exists
if [ ! -d "$SITE_DIR" ]; then
    echo "❌ Pages directory not found: $SITE_DIR"
    exit 1
fi

cd "$SITE_DIR"

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
fi

# Check for --build flag
if [ "$1" == "--build" ]; then
    echo "🔨 Building site for production..."
    npm run build
    echo ""
    echo "✅ Build complete! Output in: $SITE_DIR/dist"
    echo "   Run 'npm run preview' from pages/ to preview"
elif [ "$1" == "--preview" ]; then
    echo "🔨 Building and previewing..."
    npm run serve
else
    echo "🌐 Starting Astro dev server..."
    echo "   Site will be available at http://localhost:4321"
    echo ""
    npm run dev
fi

