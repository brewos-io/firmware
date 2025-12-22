# BrewOS Cloud Service

The BrewOS cloud service is a WebSocket relay that enables remote access to your espresso machine from anywhere in the world.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         CLOUD SERVICE                                   │
│        (WebSocket Relay + SQLite + Session-Based Auth)                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   /ws/device                              /ws/client                    │
│   (ESP32 connects here)                   (Apps connect here)           │
│         │                                        │                      │
│         ▼                                        ▼                      │
│   ┌─────────────┐                         ┌─────────────┐              │
│   │ DeviceRelay │ ◄────── messages ──────► │ ClientProxy │              │
│   └─────────────┘                         └─────────────┘              │
│         │                                        │                      │
│         └────────────────┬───────────────────────┘                      │
│                          ▼                                              │
│                    ┌───────────┐                                        │
│                    │  SQLite   │  (devices, profiles, sessions)         │
│                    └───────────┘                                        │
└─────────────────────────────────────────────────────────────────────────┘
         ▲                                        ▲
         │ WebSocket                    WebSocket │
         │                                        │
┌────────┴────────┐                    ┌─────────┴─────────┐
│     ESP32       │                    │    Web/Mobile     │
│ (your machine)  │                    │   (your app)      │
└─────────────────┘                    └───────────────────┘
```

## Key Features

- **SQLite Database** - Embedded, file-based, no external DB needed
- **Session-Based Auth** - OAuth for identity, own tokens for sessions
- **Multi-Provider OAuth** - Google and Facebook supported, easy to add more
- **QR Code Pairing** - Scan to link devices to your account
- **Device Sharing** - Share access with other users via link
- **Push Notifications** - Web push via VAPID/Web Push API
- **Admin Panel** - User, device, and session management
- **SSL/TLS Support** - Let's Encrypt auto-detection or manual certs
- **Pure WebSocket** - No MQTT dependency, deploy anywhere
- **Low Latency** - Direct message relay between clients and devices

## Authentication Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    SESSION-BASED AUTHENTICATION                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   STEP 1: Identity Verification (OAuth - ONE TIME)                      │
│   ─────────────────────────────────────────────────                     │
│   User → Google/Apple/GitHub → "This is user@example.com" ✓             │
│                                                                          │
│   STEP 2: Session Creation (Our System)                                 │
│   ──────────────────────────────────────                                │
│   POST /api/auth/google { credential: "google_id_token" }               │
│   Server: Verify with Google (one-time) → Create user → Issue tokens    │
│   Response: { accessToken, refreshToken, expiresAt, user }              │
│                                                                          │
│   STEP 3: All Subsequent API Calls                                      │
│   ─────────────────────────────────                                     │
│   Authorization: Bearer <accessToken>                                    │
│   Google is no longer involved - we control the session                 │
│                                                                          │
│   STEP 4: Token Refresh (Silent)                                        │
│   ──────────────────────────────                                        │
│   POST /api/auth/refresh { refreshToken: "..." }                        │
│   Response: { accessToken, refreshToken, expiresAt }                    │
│   Implements refresh token rotation for security                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

**Benefits of this approach:**
- **You control token expiry** - Not tied to Google's 1-hour limit
- **Multi-provider support** - All OAuth providers result in same session format
- **Revocable sessions** - Can logout users server-side at any time
- **Better security** - Refresh token rotation, constant-time comparisons

## Project Structure

```
src/cloud/
├── src/
│   ├── server.ts          # Express + WebSocket server (with SSL support)
│   ├── device-relay.ts    # ESP32 device connection handler
│   ├── client-proxy.ts    # Client app connection handler
│   ├── lib/
│   │   ├── database.ts    # SQLite database (sql.js)
│   │   └── date.ts        # UTC date utilities
│   ├── middleware/
│   │   ├── auth.ts        # Session-based auth middleware
│   │   └── admin.ts       # Admin authentication middleware
│   ├── routes/
│   │   ├── auth.ts        # Auth endpoints (Google, Facebook, refresh, logout)
│   │   ├── admin.ts       # Admin API endpoints
│   │   ├── devices.ts     # Device management API
│   │   └── push.ts        # Push notification API
│   ├── services/
│   │   ├── session.ts     # Session management (create, verify, revoke)
│   │   ├── device.ts      # Device CRUD operations
│   │   ├── admin.ts       # Admin operations (users, devices, sessions)
│   │   └── push.ts        # Push notification service
│   ├── types/
│   │   └── sql.js.d.ts    # TypeScript type definitions
│   └── types.ts           # TypeScript types
├── admin/                 # Admin UI React app
├── package.json
└── tsconfig.json
```

## Getting Started

### Prerequisites

- Node.js 18+
- npm or yarn
- Google Cloud Console account (for OAuth)

### Google OAuth Setup

> **Note for self-hosters:** Each deployment requires its own Google OAuth Client ID. OAuth credentials are per-deployment and should never be shared. For local development only, you just need `http://localhost:5173` as an authorized origin.

1. **Go to Google Cloud Console**
   - Visit [console.cloud.google.com](https://console.cloud.google.com)
   - Create a new project or select existing

2. **Configure OAuth Consent Screen**
   - Go to APIs & Services → OAuth consent screen
   - Choose "External" user type
   - Fill in app name, support email
   - Add scopes: email, profile, openid

3. **Create OAuth 2.0 Client ID**
   - Go to APIs & Services → Credentials
   - Click "Create Credentials" → "OAuth 2.0 Client ID"
   - Application type: Web application
   - Add authorized JavaScript origins:
     - `https://your-domain.com` (your production URL)
     - `http://localhost:5173` (for development)
   - Copy the **Client ID** (you'll need this for both frontend and backend)

### Installation

```bash
cd src/cloud
npm install
```

### Configuration

Create a `.env` file (see `env.example`):

```env
PORT=3001
NODE_ENV=development

# SSL/TLS Configuration (required in production)
# Option 1: Use Let's Encrypt (recommended - auto-detects certificates)
LETSENCRYPT_DOMAIN=your-domain.com

# Option 2: Manual certificate paths
SSL_CERT_PATH=/path/to/cert.pem
SSL_KEY_PATH=/path/to/key.pem

# Option 3: Reverse proxy mode (if behind nginx/Caddy/traefik with SSL termination)
TRUST_PROXY=false

# Data directory for SQLite database
DATA_DIR=./data

# Google OAuth Client ID
GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com

# Facebook OAuth (optional)
FACEBOOK_APP_ID=your-facebook-app-id
FACEBOOK_APP_SECRET=your-facebook-app-secret

# CORS
CORS_ORIGIN=http://localhost:5173

# Web UI path
WEB_DIST_PATH=../web/dist

# Push Notifications (VAPID keys)
# Auto-generated on first run if not set
VAPID_PUBLIC_KEY=
VAPID_PRIVATE_KEY=
VAPID_SUBJECT=mailto:admin@brewos.io
```

### Run Development Server

```bash
npm run dev
```

### Build for Production

```bash
npm run build
npm start
```

## Database

The service uses **SQLite** (via sql.js) for data storage.

**Data is stored in a single file:** `brewos.db`

For persistence on cloud platforms, mount a volume to `DATA_DIR`.

> 📖 **See [Database_Storage.md](./Database_Storage.md)** for complete documentation on:
> - Database schema and tables
> - Access frequency and patterns
> - What data is stored (and what is NOT stored)
> - Performance considerations
> - Backup recommendations

**Quick Summary:**
- **Stored:** User accounts, device ownership, sessions, push subscriptions
- **NOT Stored:** Machine state, statistics, brewing data (relayed in real-time only)
- **Access Frequency:** Session verification (every request), device status (on connect/disconnect), batched session updates (every 60s)

## API Endpoints

### Authentication

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/auth/google` | POST | No | Exchange Google credential for session tokens |
| `/api/auth/facebook` | POST | No | Exchange Facebook access token for session tokens |
| `/api/auth/refresh` | POST | No | Refresh session using refresh token |
| `/api/auth/logout` | POST | Yes | Revoke current session |
| `/api/auth/logout-all` | POST | Yes | Revoke all user sessions |
| `/api/auth/sessions` | GET | Yes | List active sessions |
| `/api/auth/sessions/:id` | DELETE | Yes | Revoke specific session |
| `/api/auth/me` | GET | Yes | Get current user info (includes admin status) |

### Devices

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/devices` | GET | Yes | List user's devices |
| `/api/devices/claim` | POST | Yes | Claim a device with QR token |
| `/api/devices/claim-share` | POST | Yes | Claim a device with share link |
| `/api/devices/register-claim` | POST | No | ESP32 registers claim token |
| `/api/devices/:id` | GET | Yes | Get device details |
| `/api/devices/:id` | PATCH | Yes | Update device (name, brand, model) |
| `/api/devices/:id` | DELETE | Yes | Remove device from account |
| `/api/devices/:id/share` | POST | Yes | Generate share link for device |
| `/api/devices/:id/users` | GET | Yes | List users with access to device |
| `/api/devices/:id/users/:userId` | DELETE | Yes | Revoke user's access to device |

> 📖 **See [Pairing_and_Sharing.md](./Pairing_and_Sharing.md)** for detailed documentation on device pairing and sharing flows.

### Push Notifications

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/push/vapid-key` | GET | No | Get VAPID public key for subscriptions |
| `/api/push/subscribe` | POST | Yes | Subscribe to push notifications |
| `/api/push/unsubscribe` | POST | Yes | Unsubscribe from push notifications |
| `/api/push/subscriptions` | GET | Yes | List user's push subscriptions |
| `/api/push/notify` | POST | No* | Send notification from ESP32 device |

> \* Device ID acts as authentication for the notify endpoint

### Admin (Requires Admin Role)

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/admin/stats` | GET | Admin | System-wide statistics |
| `/api/admin/users` | GET | Admin | List all users (paginated) |
| `/api/admin/users/:id` | GET | Admin | Get user details |
| `/api/admin/users/:id` | DELETE | Admin | Delete user |
| `/api/admin/users/:id/promote` | POST | Admin | Promote user to admin |
| `/api/admin/users/:id/demote` | POST | Admin | Demote user from admin |
| `/api/admin/users/:id/impersonate` | POST | Admin | Create impersonation token |
| `/api/admin/devices` | GET | Admin | List all devices (paginated) |
| `/api/admin/devices/:id` | GET | Admin | Get device details |
| `/api/admin/devices/:id` | DELETE | Admin | Force delete device |
| `/api/admin/devices/:id/disconnect` | POST | Admin | Force disconnect device WebSocket |
| `/api/admin/sessions` | GET | Admin | List all sessions (paginated) |
| `/api/admin/sessions/:id` | DELETE | Admin | Revoke any session |
| `/api/admin/connections` | GET | Admin | Get real-time WebSocket stats |

### Other

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/health` | GET | No | Health check with connection stats |
| `/api/mode` | GET | No | Returns `{ mode: 'cloud' }` |
| `/*` | GET | No | Serve web UI (SPA) |

### WebSocket

| Endpoint | Description |
|----------|-------------|
| `/ws/device` | ESP32 device connections |
| `/ws/client?token=...&device=...` | Client app connections (requires session token) |

## Device Pairing & Sharing

BrewOS supports two methods for adding devices to accounts:

### 1. ESP32 QR Pairing (Physical Access)

1. **ESP32 generates QR code** - Displays pairing URL on screen
2. **User scans QR code** - Opens `/pair` page, logs in if needed
3. **Device is claimed** - Added to user's account

### 2. User-to-User Sharing (Remote)

1. **Owner generates share link** - Via `/api/devices/:id/share`
2. **Owner shares link** - QR code, URL, or manual code
3. **Recipient claims device** - Via `/api/devices/claim-share`

> 📖 **See [Pairing_and_Sharing.md](./Pairing_and_Sharing.md)** for complete documentation including flow diagrams, API details, and security considerations.

## Session Configuration

Default session settings (configurable in `services/session.ts`):

| Setting | Default | Description |
|---------|---------|-------------|
| Access Token Expiry | 60 minutes | Short-lived for security |
| Refresh Token Expiry | 30 days | Long-lived for convenience |
| Token Size | 32 bytes (256-bit) | Cryptographically secure |

Sessions include:
- **User agent** - For "manage sessions" UI
- **IP address** - For security auditing
- **Last used** - For session activity tracking

## Deployment

### Docker

```dockerfile
FROM node:20-alpine
WORKDIR /app

# Create data directory
RUN mkdir -p /data

COPY package*.json ./
RUN npm ci --production

COPY dist ./dist

ENV DATA_DIR=/data
ENV PORT=3001

EXPOSE 3001
CMD ["node", "dist/server.js"]
```

### Platform Guides

**Railway:**
```bash
railway init
railway add
# Add volume for SQLite: /data
railway up
```

**Fly.io:**
```bash
fly launch
fly volumes create brewos_data --size 1
# Update fly.toml to mount volume
fly deploy
```

**Render:**
- Connect GitHub repo
- Add persistent disk mounted at `/data`
- Set `DATA_DIR=/data`

### Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `PORT` | No | 3001 | HTTP/WS port |
| `NODE_ENV` | No | development | Environment mode |
| `DATA_DIR` | No | `.` | Directory for SQLite database |
| `GOOGLE_CLIENT_ID` | Yes | - | Google OAuth Client ID |
| `FACEBOOK_APP_ID` | No | - | Facebook OAuth App ID |
| `FACEBOOK_APP_SECRET` | No | - | Facebook OAuth App Secret |
| `CORS_ORIGIN` | No | `*` | Allowed CORS origins |
| `WEB_DIST_PATH` | No | `../web/dist` | Path to web UI build |
| `LETSENCRYPT_DOMAIN` | No | - | Domain for auto Let's Encrypt certs |
| `SSL_CERT_PATH` | No | - | Manual SSL certificate path |
| `SSL_KEY_PATH` | No | - | Manual SSL key path |
| `SSL_CA_PATH` | No | - | Optional CA certificate path |
| `TRUST_PROXY` | No | false | Set to `true` if behind a reverse proxy |
| `VAPID_PUBLIC_KEY` | No | auto | VAPID public key for push notifications |
| `VAPID_PRIVATE_KEY` | No | auto | VAPID private key for push notifications |
| `VAPID_SUBJECT` | No | - | VAPID subject (email or URL) |

## Security Considerations

1. **Use HTTPS/WSS in production** - Terminate TLS at load balancer
2. **Session tokens are hashed** - Only hashes stored in database
3. **Refresh token rotation** - New tokens issued on each refresh
4. **Constant-time comparison** - Prevents timing attacks
5. **Session revocation** - Logout everywhere capability
6. **Rate limiting** - Add rate limiting for API endpoints (recommended)
7. **Backup SQLite** - Regular backups of `brewos.db` file

## OAuth Providers

### Implemented Providers

| Provider | Endpoint | Notes |
|----------|----------|-------|
| Google | `/api/auth/google` | Recommended, most common |
| Facebook | `/api/auth/facebook` | Requires app ID and secret |

### Adding New OAuth Providers

To add Apple Sign-In, GitHub, etc.:

1. **Create route handler** in `routes/auth.ts`:
   ```typescript
   router.post('/apple', async (req, res) => {
     const { credential } = req.body;
     // Verify with Apple
     const appleUser = await verifyAppleToken(credential);
     // Create/update profile (may link to existing user by email)
     const userId = ensureProfile(appleUser.sub, appleUser.email, ...);
     // Create session (same as Google/Facebook flow)
     const tokens = createSession(userId, metadata);
     res.json({ ...tokens, user: appleUser });
   });
   ```

2. **Add frontend button** - All providers end up with same session format

The session system is provider-agnostic - once identity is verified, our sessions work the same way regardless of which OAuth provider was used. Users signing in with different providers but the same email are automatically linked.
