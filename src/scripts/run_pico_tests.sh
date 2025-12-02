#!/bin/bash
#
# BrewOS Pico Firmware - Unit Test Runner
#
# This script builds and runs the unit tests for the Pico firmware.
# Tests run on the host machine using mock hardware implementations.
#
# Usage:
#   ./run_pico_tests.sh [--clean] [--valgrind]
#
# Options:
#   --clean     Clean build directory before building
#   --valgrind  Run tests with Valgrind memory checker
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PICO_DIR="$SCRIPT_DIR/../pico"
TEST_DIR="$PICO_DIR/test"
BUILD_DIR="$TEST_DIR/build"

# Parse arguments
CLEAN=false
VALGRIND=false

for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN=true
            shift
            ;;
        --valgrind)
            VALGRIND=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--clean] [--valgrind]"
            echo ""
            echo "Options:"
            echo "  --clean     Clean build directory before building"
            echo "  --valgrind  Run tests with Valgrind memory checker"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║       BrewOS Pico Firmware - Unit Test Runner                ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is not installed${NC}"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo -e "${RED}Error: make is not installed${NC}"
    exit 1
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${BLUE}Configuring...${NC}"
cmake .. > /dev/null 2>&1 || {
    echo -e "${RED}CMake configuration failed${NC}"
    cmake ..
    exit 1
}

# Build
echo -e "${BLUE}Building tests...${NC}"
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || {
    echo -e "${RED}Build failed${NC}"
    exit 1
}

echo ""

# Run tests
if [ "$VALGRIND" = true ]; then
    if ! command -v valgrind &> /dev/null; then
        echo -e "${RED}Error: valgrind is not installed${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}Running tests with Valgrind...${NC}"
    echo ""
    valgrind --leak-check=full --error-exitcode=1 ./brewos_tests
    RESULT=$?
else
    echo -e "${BLUE}Running tests...${NC}"
    echo ""
    ./brewos_tests
    RESULT=$?
fi

echo ""

# Report result
if [ $RESULT -eq 0 ]; then
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    ✅ ALL TESTS PASSED ✅                     ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
else
    echo -e "${RED}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║                    ❌ TESTS FAILED ❌                         ║${NC}"
    echo -e "${RED}╚══════════════════════════════════════════════════════════════╝${NC}"
fi

exit $RESULT

