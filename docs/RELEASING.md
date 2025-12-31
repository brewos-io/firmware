# BrewOS Release Management

This guide explains how to manage versions and releases for BrewOS.

## Version Scheme

BrewOS uses [Semantic Versioning](https://semver.org/):

```
MAJOR.MINOR.PATCH[-PRERELEASE]

Examples:
- 1.0.0           → Stable release
- 1.1.0-beta.1    → First beta of v1.1.0
- 1.1.0-beta.2    → Second beta
- 1.1.0-rc.1      → Release candidate
- 1.1.0           → Stable release
```

## Release Channels

### Stable Channel

- Contains only stable releases (no pre-release suffix)
- For regular users
- Tagged with `v1.0.0` format
- Created from tested beta versions

### Beta Channel

- Contains pre-release versions (`-beta.X`, `-rc.X`)
- For testers and early adopters
- Tagged with `v1.0.0-beta.1` format

### Dev Channel

- Automated builds from `main` branch on every push (to `src/esp32/`, `src/pico/`, or `src/shared/`)
- App and cloud repositories have their own release workflows
- For developers testing latest changes
- Single rolling release tagged as `dev-latest` (updated in-place, not recreated)
- Only **one** dev release exists at any time - new builds replace the previous one
- May be unstable or contain breaking changes

## Creating Releases

### Using the Version Script (Recommended)

The version script updates all version files, generates the OTA manifest, commits, tags, and pushes in one command:

```bash
# Show current version
node src/scripts/version.js

# Bump version (automatically commits, tags, pushes)
node src/scripts/version.js --bump patch --release   # 1.0.0 → 1.0.1
node src/scripts/version.js --bump minor --release   # 1.0.0 → 1.1.0
node src/scripts/version.js --bump major --release   # 1.0.0 → 2.0.0

# Set specific version
node src/scripts/version.js --set 1.1.0 --release

# Create beta release
node src/scripts/version.js --set 1.1.0-beta.1 --release

# Preview without making changes
node src/scripts/version.js --bump minor --release --dry-run
```

### Files Updated by Version Script

| File                                   | Purpose                       |
| -------------------------------------- | ----------------------------- |
| `VERSION`                              | Source of truth for version   |
| `version.json`                         | Machine-readable version info |
| `app/public/version-manifest.json` | OTA update manifest (external repository) |
| `src/pico/include/config.h`            | Pico firmware version         |
| `src/esp32/include/config.h`           | ESP32 firmware version        |
| `src/shared/protocol_defs.h`           | Protocol version              |

### What Happens After Release

1. Script commits all version files
2. Creates annotated git tag (e.g., `v1.1.0-beta.1`)
3. Pushes to GitHub with tags
4. GitHub Actions workflow triggers:
   - Builds all firmware variants
   - Creates GitHub Release with binaries
   - Deploys web app to production
5. Devices detect new version via GitHub Releases API

## Development Mode (Testing with Real Hardware)

Developer features (Dev update channel, Staging cloud) are hidden by default.

### Enabling Dev Mode

Add `?dev=true` to any URL to unlock developer options:

```
http://brewos.local/?dev=true
https://cloud.brewos.io/?dev=true
```

Once enabled, dev mode persists in localStorage until cleared.

### Option 1: OTA Updates from Dev Channel

Get automatic firmware updates whenever code is pushed to main:

1. **Enable dev mode** (visit URL with `?dev=true`)
2. **Go to Settings → System → Update Channel**
3. **Select "Dev"** (purple button)
4. **Click "Install Dev"** to flash the latest build

Every push to `main` creates a new `dev-latest` release that you can install.

### Option 2: Point to Staging Cloud

To test the web app against the staging cloud:

1. **Enable dev mode** (visit URL with `?dev=true`)
2. **Go to Settings → Cloud**
3. **Select "Staging"** environment
4. **Save** - Your device now connects to `wss://staging.brewos.io`

### Environment Options

The Cloud Settings page shows:

| Environment    | URL                       | Visibility | Use Case                      |
| -------------- | ------------------------- | ---------- | ----------------------------- |
| **Production** | `wss://cloud.brewos.io`   | Always     | Released stable/beta versions |
| **Staging**    | `wss://staging.brewos.io` | Dev mode   | Latest code from `main`       |
| **Custom**     | (user-defined)            | Always     | Local dev server or other     |

**Common custom URLs:**

- Staging: `wss://cloud-staging.brewos.io`
- Local dev: `ws://YOUR_IP:3001/ws`

### Local Development Server

To test against a local cloud server:

```bash
# Terminal 1: Run local cloud service
cd src/cloud
npm run dev  # Runs on localhost:3001

# On your ESP32, set cloud URL to:
# ws://YOUR_DEV_MACHINE_IP:3001/ws
```

### Quick Reference

```bash
# Build and flash ESP32 with latest local code
cd src/esp32
pio run -e esp32s3 -t upload

# Build and flash web UI to ESP32
cd src/web
npm run build:esp32
cd ../esp32
pio run -e esp32s3 -t uploadfs
```

---

## Typical Release Workflow

### 1. Development Phase

```bash
# Work on features in main branch
git checkout main
git pull origin main
# ... develop features ...
git commit -m "feat: new feature"
git push origin main
# ↳ Automatically deploys to STAGING
```

### 2. Test on Staging

```bash
# Visit https://staging.brewos.io to test changes
# Or flash staging firmware to test device
```

### 3. Create Beta Release

```bash
# When ready for wider testing
node src/scripts/version.js --set 1.1.0-beta.1 --release
# ↳ Builds firmware, creates GitHub release, deploys to PRODUCTION
```

### 4. Fix Issues & Iterate

```bash
# Fix bugs found in beta
git commit -m "fix: bug fix"
git push origin main
# ↳ Deploys to STAGING

# Create another beta when ready
node src/scripts/version.js --set 1.1.0-beta.2 --release
```

### 5. Stable Release

```bash
# When beta is stable and tested
node src/scripts/version.js --set 1.1.0 --release
# ↳ Deploys as stable release, all users on stable channel see update
```

## OTA Update Channels

Users can choose their update channel in Settings → System → Update Channel:

- **Stable** (default): Only receives stable releases
- **Beta**: Receives all releases including pre-releases

### How It Works

1. Device checks for updates at configured URL
2. Server returns available versions based on device's channel:
   - Stable channel → Only versions without pre-release suffix
   - Beta channel → All versions
3. Device compares version and shows update notification

### Update URL Format

```
GET /api/updates/check?current=1.0.0&channel=beta

Response:
{
  "available": true,
  "version": "1.1.0-beta.2",
  "channel": "beta",
  "releaseNotes": "...",
  "downloadUrl": "..."
}
```

## Files Modified During Release

| File                         | Contains                                |
| ---------------------------- | --------------------------------------- |
| `version.json`               | Central version info for all components |
| `src/esp32/include/config.h` | ESP32 firmware version                  |
| `app/package.json`           | App version (external repository)      |
| `cloud/package.json`         | Cloud service version (external repository) |

## GitHub Actions (CI/CD)

The CI pipeline uses two environments with automatic deployments:

### Deployment Flow

```
Push to main ─────────► Staging (staging.brewos.io)
                        └── Automatic deployment for testing

Tag v* pushed ────────► Release Workflow
                        ├── Build all firmware (Pico + ESP32)
                        ├── Create GitHub Release with binaries
                        └── Deploy to Production (cloud.brewos.io)
```

### Workflows

1. **On push to `main`** (`.github/workflows/deploy.yml`):

   - Builds and verifies all components
   - Deploys to **staging** environment automatically
   - URL: https://staging.brewos.io

2. **On tag push `v*`** (`.github/workflows/release.yml`):
   - Builds all firmware variants (Pico + ESP32)
   - Generates changelog from commits
   - Creates GitHub Release with artifacts
   - Deploys to **production** environment
   - URL: https://cloud.brewos.io

### Release Artifacts

| File                        | Description                              |
| --------------------------- | ---------------------------------------- |
| `brewos_dual_boiler.uf2`    | Pico firmware for dual boiler machines   |
| `brewos_single_boiler.uf2`  | Pico firmware for single boiler machines |
| `brewos_heat_exchanger.uf2` | Pico firmware for HX machines            |
| `brewos_esp32.bin`          | ESP32 firmware                           |
| `brewos_esp32_littlefs.bin` | ESP32 filesystem (web UI)                |
| `SHA256SUMS.txt`            | Checksums for verification               |

## GitHub Environment Setup

To enable the CI/CD pipeline, configure these environments in your GitHub repository settings.

### Creating Environments

Go to: **Repository → Settings → Environments**

#### 1. Staging Environment

Create environment named `staging`:

- **No protection rules** (auto-deploy on push)
- **Secrets required:**
  - `STAGING_SERVER_SSH_KEY` - SSH private key for deployment
  - `STAGING_SSH_HOST` - **Required if domain is behind Cloudflare**: Real server IP address (not the Cloudflare-proxied domain). If not set, defaults to `staging.brewos.io`
  - `GOOGLE_CLIENT_ID` - Google OAuth client ID

#### 2. Production Environment

Create environment named `production`:

- **Protection rules (recommended):**
  - ✓ Required reviewers (optional, for extra safety)
  - ✓ Wait timer: 0 minutes
  - ✓ Deployment branches: Only `v*` tags
- **Secrets required:**
  - `SERVER_SSH_KEY` - SSH private key for deployment
  - `SERVER_SSH_HOST` - **Required if domain is behind Cloudflare**: Real server IP address (not the Cloudflare-proxied domain). If not set, defaults to `cloud.brewos.io`
  - `GOOGLE_CLIENT_ID` - Google OAuth client ID

### Required Repository Secrets

These secrets are set at the repository level (Settings → Secrets and variables → Actions):

| Secret             | Description                               |
| ------------------ | ----------------------------------------- |
| `GOOGLE_CLIENT_ID` | Google OAuth client ID for authentication |

### Server Requirements

Both staging and production servers need:

1. Git repository cloned at `/root/brewos`
2. Docker and Docker Compose installed
3. Node.js 20+ installed
4. SSH access for the deployment user

### Testing the Pipeline

1. **Test staging deployment:**

   ```bash
   git push origin main
   # Watch: https://github.com/YOUR_REPO/actions
   ```

2. **Test release deployment:**
   ```bash
   node src/scripts/version.js --set 0.1.0-beta.1 --release
   # Creates tag, pushes, triggers release workflow
   ```

## Version in Code

### ESP32 (C++)

```cpp
#include "config.h"

// Access version
LOG_I("Version: %s", ESP32_VERSION);

// Check if beta
#if defined(ESP32_VERSION_PRERELEASE) && ESP32_VERSION_PRERELEASE[0] != '\0'
  LOG_I("Running beta version");
#endif
```

### Web (TypeScript)

```typescript
// Build-time constants defined in vite.config.ts
// Set by CI during release builds via RELEASE_VERSION env var

declare const __VERSION__: string; // e.g., "1.0.0" or "dev"
declare const __ENVIRONMENT__: string; // "staging" | "production" | "development"
declare const __ESP32__: boolean; // true when built for ESP32
declare const __CLOUD__: boolean; // true when built for cloud

// Usage
console.log(`App version: ${__VERSION__}`);
console.log(`Environment: ${__ENVIRONMENT__}`);

if (__ESP32__) {
  // ESP32-specific code
}
```

### Cloud (Node.js)

```typescript
import pkg from "../package.json";
console.log(`Version: ${pkg.version}`);
```

## Best Practices

1. **Always test beta before stable release**
2. **Write clear release notes** for each version
3. **Never modify a published release** - create a new version instead
4. **Keep version numbers synchronized** across all components
5. **Use the release script** to avoid manual errors
