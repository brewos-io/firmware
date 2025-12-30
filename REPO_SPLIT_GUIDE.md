# Repository Split Guide

This guide will help you split the `brewos-io/all` monorepo into separate repositories.

## Target Repositories

1. **`brewos-io/firmware`** - ESP32, Pico, shared code, web UI, cloud backend, scripts
2. **`brewos-io/web`** (or `brewos-io/pages`) - GitHub Pages site
3. **`brewos-io/homeassistant`** - Home Assistant integration
4. **`brewos-io/docs`** - Documentation (later)

## Prerequisites

### Option 1: Install git-filter-repo (Recommended)

```bash
# Install git-filter-repo
pip install git-filter-repo

# Or via Homebrew (macOS)
brew install git-filter-repo
```

### Option 2: Use built-in git tools

The scripts will fall back to git subtree if git-filter-repo is not available, but history preservation may be less complete.

## Step-by-Step Process

### Step 1: Create Empty Repositories on GitHub

1. Go to https://github.com/organizations/brewos-io/repositories/new
2. Create these repositories (empty, no README):
   - `firmware`
   - `web` (or `pages` - your choice)
   - `homeassistant`

### Step 2: Extract Firmware Repository

The firmware repo contains multiple directories, so it needs special handling:

```bash
# Run the firmware extraction script
./scripts/split-firmware-repo.sh
```

This will:
- Extract: `src/esp32`, `src/pico`, `src/shared`, `src/web`, `src/cloud`, `src/scripts`
- Also include: `assets/`, `docs/`, root files (LICENSE, README.md, etc.)
- Move `src/*` directories to root level in the new repo
- Preserve full git history

### Step 3: Extract Web/Pages Repository

```bash
# For the pages directory
cd .split-temp
mkdir web && cd web
git init
git remote add source ../..
git fetch source
git subtree pull --prefix=pages source main --squash
# Or if your default branch is master:
# git subtree pull --prefix=pages source master --squash

# Add GitHub remote and push
git remote add origin https://github.com/brewos-io/web.git
git push -u origin main
```

Or use the automated script:

```bash
./scripts/split-repos.sh
```

### Step 4: Extract Home Assistant Repository

```bash
cd .split-temp
mkdir homeassistant && cd homeassistant
git init
git remote add source ../..
git fetch source
git subtree pull --prefix=homeassistant source main --squash

# Add GitHub remote and push
git remote add origin https://github.com/brewos-io/homeassistant.git
git push -u origin main
```

### Step 5: Update Original Repository

After splitting, you may want to:

1. **Keep the monorepo** - Update it to reference the new repos as submodules or just remove the split directories
2. **Archive the monorepo** - If you're fully migrating to separate repos

To remove split directories from the original repo:

```bash
# Remove the directories that were split
git rm -r pages homeassistant
git rm -r src/esp32 src/pico src/shared src/web src/cloud src/scripts

# Commit the changes
git commit -m "Remove directories split into separate repositories"

# Update README to reference new repos
# Then push
git push
```

## Alternative: Using git-filter-repo Manually

If you prefer more control, you can use git-filter-repo directly:

### For Firmware:

```bash
# Clone the repo
git clone https://github.com/brewos-io/all.git firmware-temp
cd firmware-temp

# Extract only the directories we want
git filter-repo \
  --path src/esp32 \
  --path src/pico \
  --path src/shared \
  --path src/web \
  --path src/cloud \
  --path src/scripts \
  --path assets \
  --path docs \
  --path LICENSE \
  --path README.md \
  --path-rename src/esp32:esp32 \
  --path-rename src/pico:pico \
  --path-rename src/shared:shared \
  --path-rename src/web:web \
  --path-rename src/cloud:cloud \
  --path-rename src/scripts:scripts

# Add remote and push
git remote add origin https://github.com/brewos-io/firmware.git
git push -u origin main --force
```

### For Web/Pages:

```bash
git clone https://github.com/brewos-io/all.git web-temp
cd web-temp

git filter-repo --path pages --path-rename pages:.

git remote add origin https://github.com/brewos-io/web.git
git push -u origin main --force
```

### For Home Assistant:

```bash
git clone https://github.com/brewos-io/all.git homeassistant-temp
cd homeassistant-temp

git filter-repo --path homeassistant --path-rename homeassistant:.

git remote add origin https://github.com/brewos-io/homeassistant.git
git push -u origin main --force
```

## Post-Split Tasks

### 1. Update README Files

Each new repository should have an updated README that:
- Explains what the repository contains
- Links to related repositories
- Updates any paths that changed (e.g., `src/esp32` → `esp32`)

### 2. Update CI/CD

If you have GitHub Actions or other CI:
- Update paths in workflows
- Update any cross-repo references

### 3. Update Documentation

- Update links in documentation
- Update any references to the monorepo structure

### 4. Consider Git Submodules (Optional)

If you want to keep some connection between repos:

```bash
# In a new "meta" repo or in one of the repos
git submodule add https://github.com/brewos-io/firmware.git
git submodule add https://github.com/brewos-io/web.git
git submodule add https://github.com/brewos-io/homeassistant.git
```

## Troubleshooting

### "git-filter-repo: command not found"

Install it:
```bash
pip install git-filter-repo
```

### "Repository already exists" error

The scripts check if the GitHub repository exists. If it does and has content, you may need to:
- Delete and recreate it (if it's empty)
- Or manually merge the extracted history

### History looks incomplete

If using the subtree method, history may be squashed. For full history, use `git-filter-repo`.

## Recommendations

1. **Use git-filter-repo** for the best history preservation
2. **Test on a clone first** - Don't modify the original repo until you're sure
3. **Keep the original repo** as a backup initially
4. **Update all documentation** after splitting
5. **Consider a meta-repo** with submodules if you need to coordinate releases

## Quick Reference

```bash
# Install git-filter-repo
pip install git-filter-repo

# Extract firmware (handles multiple dirs)
./scripts/split-firmware-repo.sh

# Extract web and homeassistant
./scripts/split-repos.sh

# Or manually with git-filter-repo
# (see examples above)
```

