#!/bin/bash
#
# BrewOS Pull Request Verification Script
#
# This script mirrors the CI workflow (ci.yml) and runs all checks required 
# before merging a PR:
#
# 1. Build App for ESP32 (from external app repository)
# 2. Build Firmware (ESP32 + Pico)
# 3. Run Pico Unit Tests
#
# Note: App and Cloud repositories have their own CI workflows.
#
# Usage:
#   ./verify_pr.sh [--skip-firmware] [--skip-tests] [--skip-app] [--fast]
#
# Options:
#   --skip-firmware  Skip firmware builds (ESP32 + Pico)
#   --skip-tests     Skip running unit tests
#   --skip-app       Skip building app for ESP32
#   --fast           Skip firmware builds, tests, and app build
#   --help, -h       Show this help message
#

set -e  # Exit on first error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Timing
START_TIME=$(date +%s)
STEP_START_TIME=$START_TIME

# Parse arguments
SKIP_FIRMWARE=false
SKIP_TESTS=false
SKIP_APP=false

for arg in "$@"; do
    case $arg in
        --skip-firmware)
            SKIP_FIRMWARE=true
            shift
            ;;
        --skip-tests)
            SKIP_TESTS=true
            shift
            ;;
        --skip-app)
            SKIP_APP=true
            shift
            ;;
        --fast)
            SKIP_FIRMWARE=true
            SKIP_TESTS=true
            SKIP_APP=true
            shift
            ;;
        --help|-h)
            head -n 20 "$0" | tail -n +2
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Functions
print_header() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC} $1"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════════╝${NC}"
}

print_step() {
    STEP_START_TIME=$(date +%s)
    echo ""
    echo -e "${BLUE}▶ $1${NC}"
}

print_success() {
    local duration=$(($(date +%s) - STEP_START_TIME))
    echo -e "${GREEN}✓ $1${NC} ${YELLOW}(${duration}s)${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${MAGENTA}→ $1${NC}"
}

# Error handler
on_error() {
    echo ""
    print_error "Verification failed at: ${BASH_COMMAND}"
    echo ""
    print_info "To fix:"
    print_info "  1. Review the error message above"
    print_info "  2. Make necessary changes"
    print_info "  3. Run this script again"
    echo ""
    exit 1
}

trap on_error ERR

# ============================================================================
# Main Verification Steps
# ============================================================================

print_header "BrewOS Pull Request Verification (Firmware)"
echo ""
echo -e "  ${CYAN}Repository:${NC} $PROJECT_ROOT"
echo -e "  ${CYAN}Mode:${NC}       $([ "$SKIP_FIRMWARE" = true ] && echo "Fast (no firmware)" || echo "Full verification")"
echo ""

# ----------------------------------------------------------------------------
# Step 1: Build App for ESP32 (from external repository)
# ----------------------------------------------------------------------------
if [ "$SKIP_APP" = false ]; then
    print_step "1/3 Building App for ESP32 (from external app repository)..."
    APP_DIR="$PROJECT_ROOT/../app"
    
    if [ ! -d "$APP_DIR" ]; then
        print_warning "App repository not found at $APP_DIR"
        print_info "Cloning app repository..."
        cd "$PROJECT_ROOT/.."
        git clone https://github.com/brewos-io/app.git app || {
            print_error "Failed to clone app repository"
            print_info "You can skip this step with --skip-app"
            exit 1
        }
    fi
    
    cd "$SCRIPT_DIR"
    if [ -f "./build_app_for_esp32.sh" ]; then
        ./build_app_for_esp32.sh --build-only > /dev/null
        print_success "App built for ESP32"
    else
        print_warning "build_app_for_esp32.sh not found, skipping app build"
    fi
else
    print_step "1/3 Building App for ESP32..."
    print_warning "Skipped (--skip-app or --fast)"
fi

# ----------------------------------------------------------------------------
# Step 2: Build Firmware (optional)
# ----------------------------------------------------------------------------
if [ "$SKIP_FIRMWARE" = false ]; then
    print_step "2/3 Building Firmware (ESP32 + Pico)..."
    cd "$SCRIPT_DIR"
    # Run full build and let errors propagate (set -e will catch them)
    ./build_firmware.sh all
    print_success "Firmware built successfully"
else
    print_step "2/3 Building Firmware (ESP32 + Pico)..."
    print_warning "Skipped (--skip-firmware or --fast)"
fi

# ----------------------------------------------------------------------------
# Step 3: Run Unit Tests (optional)
# ----------------------------------------------------------------------------
if [ "$SKIP_TESTS" = false ]; then
    print_step "3/3 Running Pico Unit Tests..."
    cd "$SCRIPT_DIR"
    # Run tests and let errors propagate (set -e will catch them)
    ./run_pico_tests.sh
    print_success "All unit tests passed"
else
    print_step "3/3 Running Pico Unit Tests..."
    print_warning "Skipped (--skip-tests or --fast)"
fi

# ============================================================================
# Summary
# ============================================================================

TOTAL_DURATION=$(($(date +%s) - START_TIME))
MINUTES=$((TOTAL_DURATION / 60))
SECONDS=$((TOTAL_DURATION % 60))

echo ""
print_header "✅ Pull Request Verification Complete"
echo ""
echo -e "  ${GREEN}All checks passed!${NC}"
echo -e "  ${CYAN}Total time:${NC} ${MINUTES}m ${SECONDS}s"
echo ""
echo -e "  ${MAGENTA}Ready to create/merge pull request${NC}"
echo ""

# Optional suggestions based on mode
if [ "$SKIP_FIRMWARE" = true ]; then
    echo -e "  ${YELLOW}Note:${NC} Firmware builds were skipped. Run without --fast for full verification."
    echo ""
fi

exit 0

