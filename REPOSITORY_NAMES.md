# Repository Names and Descriptions

## Recommended Repository Structure

### 1. `brewos-io/firmware` ⭐

**Name:** `firmware`  
**Full Name:** `brewos-io/firmware`

**Description:**

```
Open-source firmware for espresso machine control - ESP32, Pico RP2350, web UI, and cloud backend
```

**Topics:** `esp32`, `raspberry-pi-pico`, `espresso`, `firmware`, `pid-control`, `embedded-systems`, `c`, `cpp`, `react`, `typescript`, `mqtt`, `websocket`

**Long Description:**
BrewOS firmware for espresso machine control. Includes ESP32-S3 firmware (WiFi, web server, MQTT, BLE), Pico RP2350 firmware (real-time PID control, boiler management), React web UI, cloud backend for remote access, and build scripts. Supports dual boiler, single boiler, and heat exchanger machines.

---

### 2. `brewos-io/web` ⭐ (Recommended over "pages")

**Name:** `web`  
**Full Name:** `brewos-io/web`

**Description:**

```
BrewOS marketing website and documentation site - Built with Astro
```

**Topics:** `astro`, `website`, `documentation`, `marketing`, `static-site`, `github-pages`

**Long Description:**
Public-facing website for BrewOS project. Built with Astro, deployed on GitHub Pages. Includes project documentation, getting started guides, privacy policy, terms of service, and partnership information.

**Alternative Name:** `brewos-io/site` (if you prefer a more generic name)

---

### 3. `brewos-io/homeassistant`

**Name:** `homeassistant`  
**Full Name:** `brewos-io/homeassistant`

**Description:**

```
Home Assistant integration for BrewOS espresso machines - Custom component, Lovelace card, and MQTT auto-discovery
```

**Topics:** `home-assistant`, `home-assistant-custom-integration`, `mqtt`, `espresso`, `home-automation`, `hacs`, `lovelace`

**Long Description:**
Home Assistant integration for BrewOS espresso machines. Includes native custom component with 35+ entities (sensors, switches, buttons), custom Lovelace dashboard card, MQTT auto-discovery support, example automations, and HACS compatibility.

---

### 4. `brewos-io/docs` (Future)

**Name:** `docs`  
**Full Name:** `brewos-io/docs`

**Description:**

```
BrewOS documentation - Architecture, hardware specs, API reference, and guides
```

**Topics:** `documentation`, `mkdocs`, `docusaurus`, `docs-as-code`

**Long Description:**
Comprehensive documentation for BrewOS project. Includes architecture diagrams, hardware specifications, API documentation, setup guides, compatibility information, and development documentation.

---

## Repository Settings Recommendations

### Visibility

- **firmware**: Public (main project)
- **web**: Public (marketing site)
- **homeassistant**: Public (integration)
- **docs**: Public (documentation)

### Default Branch

- Use `main` for all repositories (modern standard)

### README Files

Each repository should have a README that:

1. Explains what the repository contains
2. Links to related repositories
3. Provides quick start instructions
4. Links to main project: `brewos-io/firmware`

### Topics/Tags

Use consistent topics across repos:

- `brewos` (all repos)
- `espresso` (firmware, homeassistant)
- `open-source` (all repos)

---

## Naming Rationale

### Why `web` instead of `pages`?

- ✅ Shorter and clearer
- ✅ Less confusion with "GitHub Pages" (deployment method)
- ✅ More professional
- ✅ Better for SEO/searchability
- ✅ Can be used for other web assets in the future

### Why `firmware`?

- ✅ Clear and descriptive
- ✅ Standard naming convention
- ✅ Easy to find
- ✅ Covers all embedded code (ESP32 + Pico)

### Why `homeassistant`?

- ✅ Matches Home Assistant ecosystem naming
- ✅ Clear purpose
- ✅ Easy to discover in HACS

---

## GitHub Organization Structure

```
brewos-io/
├── firmware          (⭐ Main project)
├── web               (Marketing site)
├── homeassistant     (HA integration)
└── docs              (Documentation - future)
```

## Cross-Repository Links

Each repo should link to others:

**firmware/README.md:**

```markdown
## Related Repositories

- [Web Site](https://github.com/brewos-io/web) - Marketing and documentation site
- [Home Assistant Integration](https://github.com/brewos-io/homeassistant) - HA custom component
```

**web/README.md:**

```markdown
## Related Repositories

- [Firmware](https://github.com/brewos-io/firmware) - Main firmware repository
- [Home Assistant Integration](https://github.com/brewos-io/homeassistant) - HA integration
```

**homeassistant/README.md:**

```markdown
## Related Repositories

- [Firmware](https://github.com/brewos-io/firmware) - Main firmware repository
- [Web Site](https://github.com/brewos-io/web) - Project website
```
