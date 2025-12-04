#!/bin/bash
# Lint BrewOS Web App
# Usage: ./lint_web.sh [--fix]

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

# Check for --fix flag
if [ "$1" == "--fix" ]; then
    echo "🔧 Running ESLint with auto-fix..."
    npm run lint -- --fix
    echo ""
    echo "✅ Linting complete with auto-fixes applied!"
else
    echo "🔍 Running ESLint..."
    echo ""
    npm run lint
fi

