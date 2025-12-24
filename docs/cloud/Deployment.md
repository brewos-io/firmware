# Cloud Service Deployment Guide

This guide covers deploying the BrewOS cloud service.

## Quick Start: Automated Setup

For a fresh Ubuntu/Debian server, use the automated setup script:

```bash
# SSH into your server, then run:
curl -sSL https://raw.githubusercontent.com/mizrachiran/brewos/main/scripts/setup-server.sh | \
  bash -s -- \
    --domain cloud.example.com \
    --google-client-id "your-google-client-id.apps.googleusercontent.com" \
    --email "admin@example.com"
```

This script automatically:
- Updates the system and installs dependencies
- Configures firewall (UFW) and fail2ban
- Installs Docker and Caddy (reverse proxy with auto-HTTPS)
- Clones the repository and builds the application
- Creates Docker volumes for data persistence
- Starts the service with health checks

For staging servers, add `--staging`:
```bash
./scripts/setup-server.sh --domain staging.example.com --google-client-id "..." --email "..." --staging
```

## Manual Setup

If you prefer manual setup, follow the sections below.

## Prerequisites

1. Build the web UI:
   ```bash
   cd src/web
   npm install
   npm run build
   ```

2. Build the cloud service:
   ```bash
   cd src/cloud
   npm install
   npm run build
   ```

## Local Development

```bash
cd src/cloud
npm run dev
```

Access at `http://localhost:3001`

## Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `PORT` | No | 3001 | HTTP/WebSocket port |
| `DATA_DIR` | No | `.` | Directory for SQLite database |
| `GOOGLE_CLIENT_ID` | Yes | - | Google OAuth Client ID |
| `CORS_ORIGIN` | No | `*` | Allowed CORS origins |
| `WEB_DIST_PATH` | No | `../web/dist` | Path to web UI build |

## Docker Deployment

### Dockerfile

Create `src/cloud/Dockerfile`:

```dockerfile
# Build stage
FROM node:20-alpine AS builder
WORKDIR /app

# Copy package files
COPY package*.json ./
RUN npm ci

# Copy source and build
COPY tsconfig.json ./
COPY src ./src
RUN npm run build

# Production stage
FROM node:20-alpine
WORKDIR /app

# Create data directory for SQLite
RUN mkdir -p /data

# Copy built files
COPY --from=builder /app/dist ./dist
COPY --from=builder /app/package*.json ./
RUN npm ci --production

# Copy web UI (pre-built)
COPY ../web/dist ./web

ENV NODE_ENV=production
ENV WEB_DIST_PATH=./web
ENV DATA_DIR=/data
EXPOSE 3001

CMD ["node", "dist/server.js"]
```

### Docker Compose

```yaml
# Fixed project name ensures consistent volume naming across deployments
name: brewos

services:
  brewos-cloud:
    build:
      context: .
      dockerfile: Dockerfile
    ports:
      - "3001:3001"
    volumes:
      - brewos-data:/data
    environment:
      - PORT=3001
      - DATA_DIR=/data
      - GOOGLE_CLIENT_ID=${GOOGLE_CLIENT_ID}
    restart: unless-stopped
    stop_grace_period: 30s

volumes:
  brewos-data:
```

### Build and Run

```bash
docker build -t brewos-cloud .
docker run -p 3001:3001 \
  -v brewos-data:/data \
  -e GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com \
  brewos-cloud
```

## SSL/TLS Configuration

### Using Reverse Proxy (Nginx)

```nginx
server {
    listen 443 ssl http2;
    server_name cloud.example.com;

    ssl_certificate /etc/letsencrypt/live/cloud.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/cloud.example.com/privkey.pem;

    location / {
        proxy_pass http://localhost:3001;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

### Using Caddy (Automatic HTTPS)

**Important for ESP32 compatibility:** Use RSA certificates instead of ECDSA. ESP32 devices have hardware acceleration for RSA but not ECDSA, enabling fast SSL handshakes (~500ms vs 5-15s).

```
cloud.example.com {
    # Use RSA certificates for ESP32 compatibility
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
}
```

**Note:** After changing the certificate type, you may need to delete the old certificate cache and restart Caddy:
```bash
rm -rf /var/lib/caddy/.local/share/caddy/certificates/acme-v02.api.letsencrypt.org-directory/cloud.example.com
systemctl restart caddy
```

## Health Checks

The service exposes `/api/health` for health monitoring:

```bash
curl http://localhost:3001/api/health
```

Response:
```json
{
  "status": "ok",
  "timestamp": "2024-01-01T00:00:00.000Z",
  "devices": 5,
  "clients": 12
}
```

## Data Persistence

The SQLite database is stored in `DATA_DIR/brewos.db`. Ensure this directory is:
- Mounted as a volume in Docker
- Backed up regularly
- On persistent storage if using cloud platforms

## Backup and Restore

### Creating Backups

```bash
# Run on the server
./scripts/backup-server.sh

# Or specify output location
./scripts/backup-server.sh --output /path/to/backup.tar.gz
```

Backups include:
- Database (`brewos.db`)
- Configuration files (`.env`, `Caddyfile`)
- Backup metadata (timestamp, version)

Backups are stored in `/root/backups/` by default. The script automatically keeps only the last 7 backups.

### Restoring from Backup

```bash
./scripts/restore-server.sh /path/to/brewos_backup.tar.gz
```

### Automated Backups

Add a cron job for daily backups:

```bash
# Edit crontab
crontab -e

# Add daily backup at 3am
0 3 * * * /root/brewos/scripts/backup-server.sh >> /var/log/brewos-backup.log 2>&1
```

## Server Management Scripts

| Script | Description |
|--------|-------------|
| `scripts/setup-server.sh` | Initial server setup (fresh install) |
| `scripts/update-server.sh` | Update to latest version (created by setup script) |
| `scripts/backup-server.sh` | Create a backup |
| `scripts/restore-server.sh` | Restore from backup |

## Troubleshooting

### View Logs

```bash
cd /root/brewos/src/cloud
docker compose logs -f
```

### Restart Service

```bash
cd /root/brewos/src/cloud
docker compose restart
```

### Check Health

```bash
curl http://localhost:3001/api/health
```

### Database Issues

If the database becomes corrupted:

1. Stop the service: `docker compose down`
2. Restore from backup: `./scripts/restore-server.sh /root/backups/latest.tar.gz`
3. Start the service: `docker compose up -d`
