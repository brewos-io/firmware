#!/bin/bash
# Run BrewOS Web UI in development mode
# Usage: ./run_app.sh [--cloud]
#   --cloud: Start both cloud server and web app in cloud mode

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEB_DIR="$SCRIPT_DIR/../web"
CLOUD_DIR="$SCRIPT_DIR/../cloud"

# Check for --cloud flag
if [ "$1" == "--cloud" ]; then
    # Start both cloud server and web app
    
    # Check if cloud directory exists
    if [ ! -d "$CLOUD_DIR" ]; then
        echo "❌ Cloud directory not found: $CLOUD_DIR"
        exit 1
    fi
    
    # Check if web directory exists
    if [ ! -d "$WEB_DIR" ]; then
        echo "❌ Web directory not found: $WEB_DIR"
        exit 1
    fi
    
    # Function to cleanup background processes on exit
    cleanup() {
        echo ""
        echo "🛑 Shutting down services..."
        kill $CLOUD_PID $WEB_PID 2>/dev/null || true
        wait $CLOUD_PID $WEB_PID 2>/dev/null || true
        exit 0
    }
    trap cleanup SIGINT SIGTERM
    
    # Start cloud server in background
    echo "☁️  Starting cloud server..."
    cd "$CLOUD_DIR"
    
    # Check for .env file
    if [ ! -f ".env" ]; then
        if [ -f "env.example" ]; then
            echo "⚠️  No .env file found. Creating from env.example..."
            cp env.example .env
            echo "   Please update .env with your configuration"
        else
            echo "⚠️  No .env file found. Create one with required variables."
        fi
    fi
    
    # Install dependencies if needed
    if [ ! -d "node_modules" ]; then
        echo "📦 Installing cloud dependencies..."
        npm install
    fi
    
    # Build TypeScript
    echo "🔨 Building cloud service..."
    npm run build
    
    # Start cloud server
    npm start > /tmp/brewos-cloud.log 2>&1 &
    CLOUD_PID=$!
    echo "   Cloud server PID: $CLOUD_PID"
    echo "   Cloud server logs: tail -f /tmp/brewos-cloud.log"
    
    # Wait for cloud server to be ready (check if port 3001 is listening)
    echo "   Waiting for cloud server to start..."
    MAX_WAIT=30
    WAIT_COUNT=0
    
    # Check if curl is available, otherwise use a simpler check
    if command -v curl > /dev/null 2>&1; then
        while [ $WAIT_COUNT -lt $MAX_WAIT ]; do
            if curl -s http://localhost:3001/api/info > /dev/null 2>&1; then
                echo ""
                echo "   ✓ Cloud server is ready!"
                break
            fi
            sleep 1
            WAIT_COUNT=$((WAIT_COUNT + 1))
            echo -n "."
        done
    else
        # Fallback: just check if port is open using nc or timeout
        while [ $WAIT_COUNT -lt $MAX_WAIT ]; do
            if (command -v nc > /dev/null 2>&1 && nc -z localhost 3001 > /dev/null 2>&1) || \
               (command -v timeout > /dev/null 2>&1 && timeout 1 bash -c "echo > /dev/tcp/localhost/3001" 2>/dev/null); then
                echo ""
                echo "   ✓ Cloud server port is open!"
                sleep 2  # Give it a moment to fully initialize
                break
            fi
            sleep 1
            WAIT_COUNT=$((WAIT_COUNT + 1))
            echo -n "."
        done
    fi
    
    if [ $WAIT_COUNT -eq $MAX_WAIT ]; then
        echo ""
        echo "   ⚠️  Cloud server didn't start in time, but continuing anyway..."
        echo "   Check logs: tail -f /tmp/brewos-cloud.log"
    fi
    
    # Start web app
    echo ""
    echo "🌐 Starting web app in cloud mode..."
    cd "$WEB_DIR"
    
    # Install dependencies if needed
    if [ ! -d "node_modules" ]; then
        echo "📦 Installing web dependencies..."
        npm install
    fi
    
    echo "   Web app will be available at http://localhost:3000"
    echo "   Cloud server is running at http://localhost:3001"
    echo ""
    echo "Press Ctrl+C to stop both services"
    echo ""
    
    # Run web app in foreground (so we can see its output)
    npm run dev -- --mode cloud &
    WEB_PID=$!
    
    # Wait for both processes
    wait $CLOUD_PID $WEB_PID
else
    # Just run web app in local mode
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
    
    echo "🌐 Starting web app in local/ESP32 mode..."
    echo "   Connect to your ESP32 at http://brewos.local or its IP address"
    npm run dev
fi

