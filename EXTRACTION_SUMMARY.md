# Repository Extraction - Ready to Execute

## ✅ What's Ready

You have everything you need to extract the repositories:

### Scripts Created

1. **`scripts/extract-repos.sh`** - Main extraction script
   - Extracts `pages/` → `brewos-io/web`
   - Extracts `homeassistant/` → `brewos-io/homeassistant`
   - Adds LICENSE, README, .gitignore, workflows
   - Updates GitHub links automatically
   - Copies assets to web repo

2. **`scripts/cleanup-firmware-repo.sh`** - Cleanup script
   - Removes extracted directories from firmware repo
   - Removes pages.yml workflow
   - Updates .gitignore
   - Commits changes locally

3. **`scripts/update-github-links.sh`** - Optional helper
   - Updates GitHub links in firmware repo after extraction

### Documentation

- **`EXTRACTION_GUIDE.md`** - Complete step-by-step guide
- **`REPOSITORY_NAMES.md`** - Repository naming and descriptions
- **`REPO_SPLIT_GUIDE.md`** - Original detailed guide

## 🚀 Quick Start (3 Steps)

### Step 1: Extract Repositories

```bash
./scripts/extract-repos.sh
```

This will:
- Create extracted repos in `.extract-temp/`
- Preserve git history (if git-filter-repo is installed)
- Set up all necessary files (LICENSE, README, .gitignore, workflows)
- Update GitHub links to point to `brewos-io/firmware`

### Step 2: Push to GitHub

The script will show you the exact commands, but here they are:

```bash
# Push web repo
cd .extract-temp/web
git remote add origin https://github.com/brewos-io/web.git
git push -u origin main --force

# Push homeassistant repo
cd ../homeassistant
git remote add origin https://github.com/brewos-io/homeassistant.git
git push -u origin main --force
```

### Step 3: Clean Up Firmware Repo

```bash
cd /Users/mizrachiran/Projects/all
./scripts/cleanup-firmware-repo.sh
git push
```

## 📦 What Each Repository Gets

### `brewos-io/web`
- ✅ `pages/` directory (moved to root)
- ✅ `assets/` directory (copied)
- ✅ LICENSE
- ✅ README.md
- ✅ .gitignore
- ✅ `.github/workflows/pages.yml` (GitHub Pages deployment)
- ✅ Updated GitHub links

### `brewos-io/homeassistant`
- ✅ `homeassistant/` directory (moved to root)
- ✅ LICENSE
- ✅ README.md (updated with related repo links)
- ✅ .gitignore
- ✅ Updated GitHub links

### `brewos-io/firmware` (after cleanup)
- ✅ All firmware code (ESP32, Pico, web UI, cloud)
- ✅ `assets/` directory (kept)
- ✅ `docs/` directory (kept)
- ✅ All other files
- ❌ `pages/` removed
- ❌ `homeassistant/` removed
- ❌ `pages.yml` workflow removed

## 🔍 Verification

After extraction, verify:

1. **Web repo:**
   ```bash
   cd .extract-temp/web
   ls -la  # Should see pages files, assets, LICENSE, README, .gitignore
   git log --oneline -5  # Should see commit history
   ```

2. **Home Assistant repo:**
   ```bash
   cd .extract-temp/homeassistant
   ls -la  # Should see homeassistant files, LICENSE, README, .gitignore
   git log --oneline -5  # Should see commit history
   ```

3. **GitHub Pages:**
   - After pushing web repo, enable GitHub Pages in repo settings
   - Workflow should run automatically on next push

## ⚠️ Important Notes

1. **git-filter-repo (Optional but Recommended)**
   - Install for better history preservation: `pip install git-filter-repo`
   - Script works without it, but history may be less complete

2. **Assets Directory**
   - Copied to web repo (needed for website)
   - Kept in firmware repo (used by firmware too)

3. **GitHub Links**
   - Automatically updated in extracted repos
   - Run `./scripts/update-github-links.sh` in firmware repo after cleanup

4. **Force Push**
   - The `--force` flag is needed for initial push to empty repos
   - This is safe since the repos are new

## 🆘 If Something Goes Wrong

1. **Extraction fails:**
   - Check script output for errors
   - Original firmware repo is unchanged
   - Delete `.extract-temp/` and try again

2. **Push fails:**
   - Make sure repos exist on GitHub
   - Check you have write access
   - Verify branch name (main vs master)

3. **History looks wrong:**
   - Install git-filter-repo: `pip install git-filter-repo`
   - Delete `.extract-temp/` and re-run extraction

## 📝 Post-Extraction Checklist

- [ ] Web repo pushed to GitHub
- [ ] Home Assistant repo pushed to GitHub  
- [ ] Firmware repo cleaned up and pushed
- [ ] GitHub Pages enabled for web repo
- [ ] GitHub Pages workflow runs successfully
- [ ] Website accessible at GitHub Pages URL
- [ ] All README files have correct links
- [ ] Repository descriptions set on GitHub
- [ ] GitHub links updated in firmware repo (run update script)

## 🎯 Next Steps After Extraction

1. **Update Repository Descriptions on GitHub:**
   - **web**: "BrewOS marketing website and documentation site - Built with Astro"
   - **homeassistant**: "Home Assistant integration for BrewOS espresso machines"

2. **Enable GitHub Pages:**
   - Go to web repo → Settings → Pages
   - Source: Deploy from a branch
   - Branch: `main` / `root`

3. **Update Documentation:**
   - Update any cross-references
   - Update contribution guidelines if needed

4. **Test Everything:**
   - Website loads correctly
   - Links work
   - GitHub Pages deployment works

---

**Ready to go!** Run `./scripts/extract-repos.sh` when you're ready.

