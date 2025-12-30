# Quick Start: Repository Split

## TL;DR

```bash
# 1. Install git-filter-repo (recommended)
pip install git-filter-repo

# 2. Create empty repos on GitHub:
#    - brewos-io/firmware
#    - brewos-io/web (or brewos-io/pages)
#    - brewos-io/homeassistant

# 3. Run the scripts
./scripts/split-firmware-repo.sh
./scripts/split-simple-repos.sh
```

## What Gets Split Where?

### `brewos-io/firmware`
- `src/esp32/` → `esp32/`
- `src/pico/` → `pico/`
- `src/shared/` → `shared/`
- `src/web/` → `web/`
- `src/cloud/` → `cloud/`
- `src/scripts/` → `scripts/`
- `assets/`, `docs/`, root files (LICENSE, README.md, etc.)

### `brewos-io/web` (or `brewos-io/pages`)
- `pages/` → root of new repo

### `brewos-io/homeassistant`
- `homeassistant/` → root of new repo

## Detailed Steps

See [REPO_SPLIT_GUIDE.md](./REPO_SPLIT_GUIDE.md) for complete instructions.

## Scripts Available

1. **`split-firmware-repo.sh`** - Handles the complex firmware extraction (multiple directories)
2. **`split-simple-repos.sh`** - Extracts web/pages and homeassistant (single directories)
3. **`split-repos.sh`** - Interactive script for all repos (older version)

## Notes

- **History preservation**: Use `git-filter-repo` for best results
- **Test first**: Scripts create repos in `.split-temp/` - review before pushing
- **Backup**: Keep the original repo until you're confident everything works

