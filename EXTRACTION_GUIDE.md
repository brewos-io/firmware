# Repository Extraction Guide

This guide will help you extract the `web` and `homeassistant` repositories from the `firmware` monorepo.

## Prerequisites

1. ✅ You've renamed `all` → `firmware` 
2. ✅ You've created empty repositories on GitHub:
   - `brewos-io/web`
   - `brewos-io/homeassistant`

## Quick Start

```bash
# 1. Extract both repositories
./scripts/extract-repos.sh

# 2. Review the extracted repos in .extract-temp/

# 3. Push to GitHub (instructions will be shown)

# 4. Clean up the firmware repo
./scripts/cleanup-firmware-repo.sh
```

## Detailed Steps

### Step 1: Extract Repositories

Run the extraction script:

```bash
./scripts/extract-repos.sh
```

This will:
- Extract `pages/` → `brewos-io/web` with full git history
- Extract `homeassistant/` → `brewos-io/homeassistant` with full git history
- Copy `assets/` to the web repo
- Add LICENSE, README, .gitignore, and GitHub workflows to each repo
- Prepare everything in `.extract-temp/` for review

**Note:** If `git-filter-repo` is installed, full history will be preserved. Otherwise, a manual extraction method will be used.

### Step 2: Review Extracted Repositories

Check the extracted repositories:

```bash
# Review web repo
cd .extract-temp/web
git log --oneline -10
ls -la

# Review homeassistant repo  
cd ../homeassistant
git log --oneline -10
ls -la
```

### Step 3: Push to GitHub

#### Push Web Repository

```bash
cd .extract-temp/web
git remote add origin https://github.com/brewos-io/web.git
git push -u origin main --force
```

#### Push Home Assistant Repository

```bash
cd .extract-temp/homeassistant
git remote add origin https://github.com/brewos-io/homeassistant.git
git push -u origin main --force
```

### Step 4: Clean Up Firmware Repository

After successfully pushing both repositories, clean up the firmware repo:

```bash
./scripts/cleanup-firmware-repo.sh
```

This will:
- Remove `pages/` directory
- Remove `homeassistant/` directory
- Remove `.github/workflows/pages.yml`
- Update `.gitignore`
- Commit the changes locally

Then push:

```bash
git push
```

## What Gets Extracted

### Web Repository (`brewos-io/web`)

**Extracted:**
- `pages/` → root of web repo
- `assets/` → copied to web repo
- LICENSE
- README.md (created)
- .gitignore (created)
- `.github/workflows/pages.yml` (created)

**GitHub Workflow:**
- Automatically builds and deploys to GitHub Pages on push to `main`

### Home Assistant Repository (`brewos-io/homeassistant`)

**Extracted:**
- `homeassistant/` → root of homeassistant repo
- LICENSE
- README.md (updated with related repo links)
- .gitignore (created)

### Firmware Repository (after cleanup)

**Removed:**
- `pages/` directory
- `homeassistant/` directory
- `.github/workflows/pages.yml`

**Kept:**
- `assets/` (still used by firmware)
- All other directories and files

## Troubleshooting

### "git-filter-repo: command not found"

Install it for better history preservation:

```bash
pip install git-filter-repo
```

The script will still work without it, but history may be less complete.

### "Repository already exists" error

If the GitHub repositories already have content:

1. Delete and recreate them (if empty/initial commit only)
2. Or manually merge the extracted history

### Assets not showing on website

The web repo workflow copies `assets/` to `public/assets/` during build. Make sure:
- Assets directory exists in the web repo
- Workflow runs successfully
- GitHub Pages is enabled for the repository

### History looks incomplete

If using the manual extraction method (without git-filter-repo), history may be squashed. For full history:
1. Install git-filter-repo: `pip install git-filter-repo`
2. Re-run the extraction script

## Post-Extraction Tasks

### 1. Update README Links

Update any links in the firmware README that point to the old monorepo structure.

### 2. Update GitHub Repository Descriptions

Set descriptions on GitHub:
- **web**: "BrewOS marketing website and documentation site - Built with Astro"
- **homeassistant**: "Home Assistant integration for BrewOS espresso machines - Custom component, Lovelace card, and MQTT auto-discovery"

### 3. Enable GitHub Pages

For the web repository:
1. Go to Settings → Pages
2. Source: Deploy from a branch
3. Branch: `main` / `root`
4. Save

### 4. Update Cross-References

Update any documentation or code that references:
- Old repository structure
- Internal links between components

## Verification Checklist

- [ ] Web repository pushed to GitHub
- [ ] Home Assistant repository pushed to GitHub
- [ ] GitHub Pages workflow runs successfully for web repo
- [ ] Website is accessible at the GitHub Pages URL
- [ ] Firmware repo cleaned up and pushed
- [ ] All repositories have proper README, LICENSE, .gitignore
- [ ] Cross-repository links updated

## Need Help?

If something goes wrong:
1. Check the script output for errors
2. Review the extracted repos in `.extract-temp/`
3. The original firmware repo is unchanged until you run the cleanup script
4. You can re-run the extraction if needed

