#!/bin/bash
# Run BrewOS Storybook - Component library and design system
# Usage: ./run_storybook.sh [--build]

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

# Check for --build flag
if [ "$1" == "--build" ]; then
    echo "🔨 Building Storybook for production..."
    npm run build-storybook
    echo ""
    echo "✅ Build complete! Output in: $WEB_DIR/storybook-static"
    echo "   Serve with: npx serve storybook-static"
else
    echo "📚 Starting Storybook..."
    echo "   Component library will be available at http://localhost:6006"
    echo ""
    echo "   🎨 Use the theme selector in the toolbar to switch themes"
    echo ""
    npm run storybook
fi

