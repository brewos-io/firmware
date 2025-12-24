#!/bin/bash
#
# BrewOS Server Setup Script
# 
# This script sets up a fresh Ubuntu/Debian server with everything needed
# to run the BrewOS cloud service.
#
# Usage:
#   curl -sSL https://raw.githubusercontent.com/mizrachiran/brewos/main/scripts/setup-server.sh | bash -s -- \
#     --domain cloud.brewos.io \
#     --google-client-id "your-google-client-id.apps.googleusercontent.com" \
#     --email "admin@brewos.io"
#
# Or run locally:
#   ./scripts/setup-server.sh --domain cloud.brewos.io --google-client-id "..." --email "..."
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Default values
DOMAIN=""
GOOGLE_CLIENT_ID=""
ADMIN_EMAIL=""
REPO_URL="https://github.com/mizrachiran/brewos.git"
INSTALL_DIR="/root/brewos"
DATA_DIR="/data"
ENVIRONMENT="production"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --domain)
            DOMAIN="$2"
            shift 2
            ;;
        --google-client-id)
            GOOGLE_CLIENT_ID="$2"
            shift 2
            ;;
        --email)
            ADMIN_EMAIL="$2"
            shift 2
            ;;
        --staging)
            ENVIRONMENT="staging"
            shift
            ;;
        --help)
            echo "Usage: $0 --domain <domain> --google-client-id <id> --email <email> [--staging]"
            echo ""
            echo "Options:"
            echo "  --domain           Domain name (e.g., cloud.brewos.io)"
            echo "  --google-client-id Google OAuth Client ID"
            echo "  --email            Admin email for SSL certificates"
            echo "  --staging          Set up as staging server"
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Validate required arguments
if [[ -z "$DOMAIN" ]]; then
    log_error "Missing required argument: --domain"
    exit 1
fi

if [[ -z "$GOOGLE_CLIENT_ID" ]]; then
    log_error "Missing required argument: --google-client-id"
    exit 1
fi

if [[ -z "$ADMIN_EMAIL" ]]; then
    log_error "Missing required argument: --email"
    exit 1
fi

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    log_error "This script must be run as root"
    exit 1
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              BrewOS Server Setup Script                   ║"
echo "╠═══════════════════════════════════════════════════════════╣"
echo "║  Domain:      $DOMAIN"
echo "║  Environment: $ENVIRONMENT"
echo "║  Install Dir: $INSTALL_DIR"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# ============================================================================
# STEP 1: System Updates and Basic Packages
# ============================================================================
log_info "Step 1/8: Updating system and installing basic packages..."

apt-get update -qq
apt-get upgrade -y -qq
apt-get install -y -qq \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg \
    lsb-release \
    git \
    ufw \
    fail2ban \
    unattended-upgrades \
    htop \
    vim \
    jq

log_success "System updated and basic packages installed"

# ============================================================================
# STEP 2: Security Hardening
# ============================================================================
log_info "Step 2/8: Configuring security settings..."

# Configure UFW firewall
ufw --force reset
ufw default deny incoming
ufw default allow outgoing
ufw allow ssh
ufw allow http
ufw allow https
ufw --force enable

# Configure fail2ban
cat > /etc/fail2ban/jail.local << 'EOF'
[DEFAULT]
bantime = 3600
findtime = 600
maxretry = 5
ignoreip = 127.0.0.1/8

[sshd]
enabled = true
port = ssh
filter = sshd
logpath = /var/log/auth.log
maxretry = 3
bantime = 86400
EOF

systemctl enable fail2ban
systemctl restart fail2ban

# SSH hardening (only if not already hardened)
if ! grep -q "^PasswordAuthentication no" /etc/ssh/sshd_config; then
    log_info "Hardening SSH configuration..."
    
    # Backup original config
    cp /etc/ssh/sshd_config /etc/ssh/sshd_config.bak
    
    # Apply hardening
    sed -i 's/^#*PasswordAuthentication.*/PasswordAuthentication no/' /etc/ssh/sshd_config
    sed -i 's/^#*PermitEmptyPasswords.*/PermitEmptyPasswords no/' /etc/ssh/sshd_config
    sed -i 's/^#*PubkeyAuthentication.*/PubkeyAuthentication yes/' /etc/ssh/sshd_config
    sed -i 's/^#*X11Forwarding.*/X11Forwarding no/' /etc/ssh/sshd_config
    sed -i 's/^#*MaxAuthTries.*/MaxAuthTries 3/' /etc/ssh/sshd_config
    
    # Validate config before restarting
    if sshd -t; then
        systemctl restart sshd
    else
        log_warn "SSH config validation failed, reverting..."
        cp /etc/ssh/sshd_config.bak /etc/ssh/sshd_config
    fi
fi

# Enable automatic security updates
cat > /etc/apt/apt.conf.d/20auto-upgrades << 'EOF'
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Unattended-Upgrade "1";
APT::Periodic::AutocleanInterval "7";
EOF

log_success "Security settings configured"

# ============================================================================
# STEP 3: Install Docker
# ============================================================================
log_info "Step 3/8: Installing Docker..."

if ! command -v docker &> /dev/null; then
    # Add Docker's official GPG key
    install -m 0755 -d /etc/apt/keyrings
    curl -fsSL https://download.docker.com/linux/$(. /etc/os-release && echo "$ID")/gpg | \
        gpg --dearmor -o /etc/apt/keyrings/docker.gpg
    chmod a+r /etc/apt/keyrings/docker.gpg

    # Add the repository
    echo \
        "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
        https://download.docker.com/linux/$(. /etc/os-release && echo "$ID") \
        $(lsb_release -cs) stable" | \
        tee /etc/apt/sources.list.d/docker.list > /dev/null

    # Install Docker
    apt-get update -qq
    apt-get install -y -qq docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

    # Enable and start Docker
    systemctl enable docker
    systemctl start docker
    
    log_success "Docker installed"
else
    log_info "Docker already installed, skipping..."
fi

# ============================================================================
# STEP 4: Install Caddy (Reverse Proxy with automatic HTTPS)
# ============================================================================
log_info "Step 4/8: Installing Caddy..."

if ! command -v caddy &> /dev/null; then
    apt-get install -y -qq debian-keyring debian-archive-keyring apt-transport-https
    curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | \
        gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
    curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | \
        tee /etc/apt/sources.list.d/caddy-stable.list
    apt-get update -qq
    apt-get install -y -qq caddy
    
    log_success "Caddy installed"
else
    log_info "Caddy already installed, skipping..."
fi

# Configure Caddy
cat > /etc/caddy/Caddyfile << EOF
$DOMAIN {
    # Use RSA certificates for ESP32 compatibility
    # ESP32 has hardware acceleration for RSA but not ECDSA
    # RSA certificates enable fast SSL handshakes (~500ms vs 5-15s)
    tls {
        key_type rsa2048
    }

    reverse_proxy localhost:3001 {
        # Longer timeouts for WebSocket connections
        transport http {
            read_timeout 0
            write_timeout 0
            response_header_timeout 0
        }
    }

    # WebSocket support
    @websocket {
        header Connection *Upgrade*
        header Upgrade websocket
    }
    reverse_proxy @websocket localhost:3001

    # Security headers
    header {
        X-Content-Type-Options nosniff
        X-Frame-Options DENY
        Referrer-Policy strict-origin-when-cross-origin
        -Server
    }

    # Logging
    log {
        output file /var/log/caddy/$DOMAIN.log
        format json
    }
}
EOF

mkdir -p /var/log/caddy
systemctl enable caddy
systemctl restart caddy

log_success "Caddy configured for $DOMAIN"

# ============================================================================
# STEP 5: Clone BrewOS Repository
# ============================================================================
log_info "Step 5/8: Cloning BrewOS repository..."

if [[ -d "$INSTALL_DIR" ]]; then
    log_info "Repository already exists, updating..."
    cd "$INSTALL_DIR"
    git fetch origin main
    git reset --hard origin/main
    git clean -fd
else
    git clone "$REPO_URL" "$INSTALL_DIR"
    cd "$INSTALL_DIR"
fi

log_success "Repository ready at $INSTALL_DIR"

# ============================================================================
# STEP 6: Create Data Directory and Docker Volume
# ============================================================================
log_info "Step 6/8: Setting up data directory and Docker volume..."

# Create data directory on host (for backups)
mkdir -p "$DATA_DIR"

# Create the Docker volume
docker volume create brewos_brewos-data || true

log_success "Data directory and Docker volume created"

# ============================================================================
# STEP 7: Configure Environment Files
# ============================================================================
log_info "Step 7/8: Creating environment configuration..."

# Determine WebSocket URL based on domain
WS_URL="wss://$DOMAIN/ws"

# Web UI .env
cat > "$INSTALL_DIR/src/web/.env" << EOF
VITE_GOOGLE_CLIENT_ID=$GOOGLE_CLIENT_ID
VITE_CLOUD_WS_URL=$WS_URL
VITE_ENVIRONMENT=$ENVIRONMENT
EOF

# Cloud service .env
cat > "$INSTALL_DIR/src/cloud/.env" << EOF
GOOGLE_CLIENT_ID=$GOOGLE_CLIENT_ID
PORT=3001
DATA_DIR=/data
CORS_ORIGIN=https://$DOMAIN
VAPID_SUBJECT=mailto:$ADMIN_EMAIL
EOF

log_success "Environment files created"

# ============================================================================
# STEP 8: Build and Start Services
# ============================================================================
log_info "Step 8/8: Building and starting BrewOS..."

cd "$INSTALL_DIR"

# Install Node.js for building (if not present)
if ! command -v node &> /dev/null; then
    log_info "Installing Node.js..."
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
    apt-get install -y -qq nodejs
fi

# Build web UI
log_info "Building web UI..."
cd "$INSTALL_DIR/src/web"
npm ci --silent
npm run build

# Copy web UI to cloud service
cp -r dist ../cloud/web

# Build and start Docker container
log_info "Building Docker container..."
cd "$INSTALL_DIR/src/cloud"
docker compose build

log_info "Starting BrewOS..."
docker compose up -d

# Wait for service to be healthy
log_info "Waiting for service to start..."
sleep 5

for i in 1 2 3 4 5; do
    if curl -sf http://localhost:3001/api/health > /dev/null; then
        break
    fi
    log_info "Waiting... ($i/5)"
    sleep 3
done

# Final health check
if curl -sf http://localhost:3001/api/health > /dev/null; then
    log_success "BrewOS is running!"
else
    log_error "Health check failed!"
    docker compose logs --tail=50
    exit 1
fi

# ============================================================================
# Setup Complete
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              BrewOS Setup Complete! 🎉                    ║"
echo "╠═══════════════════════════════════════════════════════════╣"
echo "║                                                           ║"
echo "║  Your BrewOS instance is now running at:                  ║"
echo "║  https://$DOMAIN"
echo "║                                                           ║"
echo "║  Service Status:                                          ║"
echo "║  - Docker: $(systemctl is-active docker)"
echo "║  - Caddy:  $(systemctl is-active caddy)"
echo "║  - BrewOS: $(docker ps --filter "name=brewos" --format "{{.Status}}" | head -1)"
echo "║                                                           ║"
echo "║  Useful Commands:                                         ║"
echo "║  - View logs:    cd $INSTALL_DIR/src/cloud && docker compose logs -f"
echo "║  - Restart:      cd $INSTALL_DIR/src/cloud && docker compose restart"
echo "║  - Update:       git pull && docker compose up -d --build"
echo "║                                                           ║"
echo "║  Data is stored in Docker volume: brewos_brewos-data      ║"
echo "║                                                           ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Create update script
cat > "$INSTALL_DIR/scripts/update-server.sh" << 'UPDATEEOF'
#!/bin/bash
# Quick update script for BrewOS
set -e

cd /root/brewos
git fetch origin main
git reset --hard origin/main
git clean -fd

cd src/web
npm ci
npm run build
cp -r dist ../cloud/web

cd ../cloud
docker compose build
docker compose down
docker compose up -d

echo "Update complete!"
docker compose logs --tail=10
UPDATEEOF

chmod +x "$INSTALL_DIR/scripts/update-server.sh"

log_success "Setup complete! Your server is ready."

