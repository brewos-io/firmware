# Machine Compatibility

This page lists machines tested with BrewOS and those expected to be compatible.

---

## ✅ Validated Machines

Machines that have been fully tested and confirmed working.

| Brand | Model | Boiler Type | Configuration | Notes |
|-------|-------|-------------|---------------|-------|
| ECM | **Synchronika** | Dual Boiler | `dual_boiler` | **Reference machine** - Primary development platform |

---

## 🔷 Same Platform (Expected Compatible)

These machines use the **same PID controller and GICAR board** as the Synchronika. They should work with minimal or no modification, but have not been individually tested yet.

> **We need testers!** If you own one of these machines, please help us validate it. See [TESTERS.md](../TESTERS.md).

### ECM (Same Platform as Synchronika)

| Model | Boiler Type | Configuration | Status | Notes |
|-------|-------------|---------------|--------|-------|
| Barista | Single Boiler | `single_boiler` | 🔷 Expected | Same GICAR board |
| Technika | Heat Exchanger | `heat_exchanger` | 🔷 Expected | Same GICAR board |
| Technika Profi | Heat Exchanger | `heat_exchanger` | 🔷 Expected | Same GICAR board |
| Mechanika | Heat Exchanger | `heat_exchanger` | 🔷 Expected | Same GICAR board |
| Mechanika Profi | Heat Exchanger | `heat_exchanger` | 🔷 Expected | Rotary pump variant |
| Mechanika V Slim | Heat Exchanger | `heat_exchanger` | 🔷 Expected | Slim form factor |
| Controvento | Dual Boiler | `dual_boiler` | 🔷 Expected | Premium model |

### Profitec (Same Platform)

ECM and Profitec share the same parent company (ECM Manufacture GmbH) and use identical electronics.

| Model | Boiler Type | Configuration | Status | Notes |
|-------|-------------|---------------|--------|-------|
| Pro 300 | Single Boiler | `dual_boiler` | 🔷 Expected | Same GICAR board |
| Pro 500 | Heat Exchanger | `heat_exchanger` | 🔷 Expected | Same GICAR board |
| Pro 700 | Dual Boiler | `dual_boiler` | 🔷 Expected | Same GICAR board |

---

## 🔲 Other Machines (Untested)

These machines use different electronics but standard E61 architecture. They may require custom wiring configuration:

### ECM

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| Classika | Single Boiler | `single_boiler` | Similar to Barista |
| Puristika | Single Boiler | `single_boiler` | Compact design |
| Casa V | Heat Exchanger | `heat_exchanger` | |

### Profitec

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| Pro 400 | Single Boiler | `single_boiler` | |
| Pro 600 | Dual Boiler | `dual_boiler` | Similar to Pro 700 |
| Pro 800 | Dual Boiler | `dual_boiler` | Lever machine |

### Lelit

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| Victoria | Single Boiler | `single_boiler` | E61 group |
| MaraX | Heat Exchanger | `heat_exchanger` | Unique HX design |
| Elizabeth | Dual Boiler | `dual_boiler` | |
| Bianca | Dual Boiler | `dual_boiler` | Flow control |

### Rocket

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| Appartamento | Heat Exchanger | `heat_exchanger` | |
| Mozzafiato | Heat Exchanger | `heat_exchanger` | |
| R58 | Dual Boiler | `dual_boiler` | |
| R Nine One | Dual Boiler | `dual_boiler` | |

### Bezzera

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| BZ10 | Heat Exchanger | `heat_exchanger` | |
| BZ13 | Heat Exchanger | `heat_exchanger` | |
| Duo | Dual Boiler | `dual_boiler` | |
| Matrix | Dual Boiler | `dual_boiler` | |

### Quickmill

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| Andreja Premium | Heat Exchanger | `heat_exchanger` | E61 group |
| Vetrano 2B | Dual Boiler | `dual_boiler` | |

### La Marzocco

| Model | Boiler Type | Expected Config | Notes |
|-------|-------------|-----------------|-------|
| Linea Mini | Dual Boiler | `dual_boiler` | May need custom wiring |
| GS3 | Dual Boiler | `dual_boiler` | May need custom wiring |

---

## Machine Types

BrewOS supports three main machine architectures:

### Single Boiler (`single_boiler`)
- One boiler for both brewing and steaming
- Temperature surfing required
- Simpler wiring

### Heat Exchanger (`heat_exchanger`)
- Steam boiler with heat exchanger for brew water
- Simultaneous brew and steam capability
- Common in mid-range machines

### Dual Boiler (`dual_boiler`)
- Separate brew and steam boilers
- Independent temperature control
- Premium machines

---

## Wiring Compatibility

### E61 Group Head
Most compatible machines use the E61 group head, which has standardized dimensions and mounting. Common features:
- 58mm portafilter
- Thermosyphon circulation
- 3-way solenoid valve

### Sensors
BrewOS uses standard sensors that fit most machines:
- **NTC Thermistors**: 100K or 50K, M4 thread
- **Thermocouple**: K-type, M4 thread for group head
- **Pressure Transducer**: 0-16 bar, 1/8" BSP

### Relays
Standard solid-state and mechanical relays are used:
- **SSR**: 25A for heaters
- **Mechanical**: 10A for pump, solenoid, etc.

---

## Adding a New Machine

To add support for a new machine:

1. **Check compatibility** - E61 group, standard sensors
2. **Document wiring** - Take photos, create diagram
3. **Create configuration** - Use existing config as template
4. **Test thoroughly** - All features, safety systems
5. **Submit validation** - See [TESTERS.md](../TESTERS.md)

See [Machine Configurations](pico/Machine_Configurations.md) for technical details.

---

## Want Your Machine Tested?

We're looking for testers! See [TESTERS.md](../TESTERS.md) to learn how you can help validate your machine.

