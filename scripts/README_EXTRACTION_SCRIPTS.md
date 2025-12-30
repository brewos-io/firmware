# Repository Extraction Scripts

These scripts were used to extract the `web` and `homeassistant` repositories from the firmware monorepo.

## Scripts

- **`extract-repos.sh`** - Main extraction script (used to extract web and homeassistant)
- **`cleanup-firmware-repo.sh`** - Cleanup script (removes extracted directories)
- **`update-github-links.sh`** - Updates GitHub repository links
- **`split-*.sh`** - Alternative extraction methods (not used, kept for reference)

## Status

✅ Extraction completed successfully
- `brewos-io/web` - Extracted and pushed
- `brewos-io/homeassistant` - Extracted and pushed
- `brewos-io/firmware` - Cleaned up

These scripts are kept for reference but are no longer needed for day-to-day development.

