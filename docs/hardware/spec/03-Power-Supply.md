# Power Supply Design

## AC/DC Isolated Power Module

Use an integrated isolated AC/DC converter module for safety and simplicity.

### Power Budget Analysis

| Consumer            | Typical    | Peak       | Notes                        |
| ------------------- | ---------- | ---------- | ---------------------------- |
| RP2354 MCU          | 50mA       | 100mA      | Via VSYS (5V) → IOVDD (3.3V) |
| RP2354 Core (DVDD)  | 30mA       | 60mA       | 1.1V via internal VREG       |
| Relay coils (×3)    | 80mA       | 150mA      | K2:70mA, K1/K3:40mA each     |
| SSR drivers (×2)    | 10mA       | 20mA       | Transistor current           |
| ESP32 module        | 150mA      | 500mA      | **WiFi TX spikes!**          |
| Indicator LEDs (×6) | 30mA       | 60mA       | 3 relay + 2 SSR + 1 status   |
| Buzzer              | 5mA        | 30mA       | When active                  |
| 3.3V Buck load      | 30mA       | 100mA      | Sensors, margin              |
| **TOTAL**           | **~355mA** | **~910mA** |                              |

**Minimum: 1.5A, Selected: Hi-Link HLK-15M05C (3A/15W)** - 3× headroom over 1A peak

### AC/DC Module Selection

| Parameter          | **HLK-15M05C**  | Mean Well IRM-20-5 | HLK-5M05 (1A)    |
| ------------------ | --------------- | ------------------ | ---------------- |
| Output Voltage     | 5V DC ±2%       | 5V DC ±5%          | 5V DC ±2%        |
| **Output Current** | **3A (15W)**    | 4A (20W)           | 1A (5W)          |
| Input Voltage      | 100-240V AC     | 85-264V AC         | 100-240V AC      |
| Isolation          | 3000V AC        | 3000V AC           | 3000V AC         |
| Efficiency         | 82%             | 87%                | 80%              |
| **Package**        | **48×28×18mm**  | 52×27×24mm         | 34×20×15mm       |
| Safety             | CE              | UL, CE, CB         | CE               |
| **Recommendation** | **Best choice** | Alternative        | **Insufficient** |

**Selected: Hi-Link HLK-15M05C** (3A/15W, adequate for ~1.1A peak load with 3× headroom)

### Power Module Schematic

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                           AC/DC POWER SUPPLY SECTION                            │
│                        ⚠️  HIGH VOLTAGE - MAINS CONNECTED ⚠️                     │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│                                                                                 │
│    MAINS INPUT                    ISOLATED MODULE                    OUTPUT    │
│    ───────────                    ───────────────                    ──────    │
│                                                                                 │
│    L (Live) ────┬────[F1]────┬────────────────────────────────┐               │
│                 │   (10A)    │                                │               │
│            ┌────┴────┐  ┌────┴────┐     ┌─────────────────┐  │               │
│            │  MOV    │  │   X2    │     │                 │  │               │
│            │ 275V    │  │  100nF  │     │   HLK-15M05C    │  │               │
│            │ (RV1)   │  │  (C1)   │     │   or similar    │  │               │
│            └────┬────┘  └────┬────┘     │                 │  │               │
│                 │            │          │  L ─────────────┼──┘               │
│                 │            │          │                 │                   │
│    N (Neutral) ─┴────────────┴──────────┤  N ─────────────┼──┐               │
│                                         │                 │  │               │
│                                         │      ═══════    │  │    ┌────────┐ │
│                                         │   (Isolation)   │  │    │  5V    │ │
│                                         │      ═══════    │  │    │  Rail  │ │
│                                         │                 │  │    │        │ │
│                                         │  +5V ───────────┼──┼───►│ +5V DC │ │
│    ⚠️ NOTE: PE (Protective Earth)      │                 │  │    │        │ │
│    connection REMOVED from PCB.        │  GND ───────────┼──┴───►│  GND   │ │
│    HV section is floating.             │                 │       │        │ │
│                                         └─────────────────┘       └────────┘ │
│                                                                                 │
│    Component Details:                                                          │
│    ─────────────────                                                           │
│    F1: Fuse, 10A 250VAC, SMD Nano² (Littelfuse 463), slow-blow (relay-switched loads only)   │
│    F2: Fuse, 2A 250VAC, SMD Nano² (Littelfuse 463), slow-blow (HLK module protection)        │
│    RV1: MOV/Varistor, 275V AC, 14mm disc (surge protection)                   │
│    C1: X2 safety capacitor, 100nF 275V AC (EMI filter)                        │
│                                                                                 │
└────────────────────────────────────────────────────────────────────────────────┘
```

## 3.3V Synchronous Buck Converter

Uses a synchronous buck converter for high efficiency and minimal heat dissipation,
critical for reliable operation inside hot espresso machine enclosures.

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                      3.3V SYNCHRONOUS BUCK CONVERTER                            │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│      5V Rail                                                     3.3V Rail     │
│         │                                                            │         │
│    ┌────┴────┐     ┌────────────────────────────┐              ┌────┴────┐    │
│    │  22µF   │     │      TPS563200DDCR         │              │  22µF   │    │
│    │  25V    │     │   Synchronous Buck 3A      │              │  10V    │    │
│    │ Ceramic │     │                            │              │ Ceramic │    │
│    │  (C3)   │     │  VIN              VOUT ────┼──[L1 2.2µH]──┤  (C4)   │    │
│    └────┬────┘     │                            │              │         │    │
│         │      ┌──►│  EN               SW ──────┘              └────┬────┘    │
│         │      │   │                            │                    │         │
│         │      │   │  FB ◄──────────────────────┼────────────────────┼────┐   │
│         │      │   │                            │              ┌────┴────┐│   │
│         │      │   │         GND                │              │  22µF   ││   │
│         │      │   └──────────┬─────────────────┘              │  10V    ││   │
│         │      │              │                                │ (C4A)   ││   │
│         └──────┘              │                                └────┬────┘│   │
│                               │                                     │     │   │
│        GND                   GND                                   GND    │   │
│                                                                            │   │
│    Feedback Network (sets 3.3V output):                                   │   │
│                           ┌────┴────┐                                     │   │
│                           │  33kΩ   │  R_FB1                              │   │
│                           │   1%    │                                     │   │
│                           └────┬────┘                                     │   │
│                                │                                          │   │
│                           FB ──┴──────────────────────────────────────────┘   │
│                                │                                              │
│                           ┌────┴────┐                                         │
│                           │  10kΩ   │  R_FB2                                  │
│                           │   1%    │                                         │
│                           └────┬────┘                                         │
│                                │                                              │
│                               GND                                             │
│                                                                                 │
└────────────────────────────────────────────────────────────────────────────────┘
```

### Why Buck Instead of LDO?

| Parameter         | LDO (AP2112K-3.3)     | Buck (TPS563200) |
| ----------------- | --------------------- | ---------------- |
| Efficiency        | ~34% at 250mA         | >90%             |
| Power Dissipation | 0.425W (dangerous!)   | ~25mW            |
| Junction Temp     | 166°C (exceeds limit) | Safe margin      |

**LDO Problem:** P_dissipated = (5V - 3.3V) × I_load = 1.7V × 250mA = 0.425W

In a hot espresso machine environment (60°C ambient), this causes thermal runaway. The buck converter solves this with 10× lower heat dissipation.

### Output Voltage Calculation

```
V_OUT = 0.768V × (1 + R_FB1/R_FB2)
V_OUT = 0.768V × (1 + 33kΩ/10kΩ) = 0.768V × 4.3 = 3.30V ✓
```

### RP2354 Internal Regulator Configuration

The RP2354 has an internal regulator that can run in LDO or SMPS (switching) mode. This design uses **SMPS mode** for efficiency, matching the original Pico 2 module's approach.

**Power Architecture:**

- **VREG_IN:** Connect to +3.3V (from TPS563200 output)
- **VREG_OUT:** Connect to **DVDD** (Core voltage pins, 1.1V)
- **LX:** Connect **2.2µH inductor** (L2) to VREG_OUT/DVDD
- **IOVDD:** All GPIO pins require 3.3V (from TPS563200)
- **Decoupling:** Place 100nF caps close to every IOVDD pin; at least one 1µF/10µF on DVDD rail

**Internal Regulator Benefits:**

- Efficient SMPS mode reduces heat dissipation
- Integrated design eliminates external 1.1V regulator
- Matches Pico 2 power architecture for compatibility

## Precision ADC Voltage Reference (Buffered)

**Purpose:** Provides stable, low-drift reference for NTC temperature measurements.

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                  PRECISION ADC VOLTAGE REFERENCE (BUFFERED)                     │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│      3.3V Rail                                                                  │
│         │                                                                       │
│    ┌────┴────┐                                                                  │
│    │   1kΩ   │  R7 - Bias resistor                                             │
│    │   1%    │  I_bias = (3.3V - 3.0V) / 1kΩ = 0.3mA                          │
│    └────┬────┘                                                                  │
│         │                                                                       │
│         ├────────────────────────► LM4040_VREF (3.0V, unloaded)               │
│         │                                 │                                     │
│    ┌────┴────┐                     ┌──────┴──────┐                             │
│    │ LM4040  │                     │    U9A      │  OPA2342UA (half)           │
│    │  3.0V   │  U5                 │   Buffer    │                             │
│    │ Shunt   │                     │  (+) ───────┤  Unity-gain follower        │
│    │  Ref    │                     │             │  Vout = Vin                 │
│    └────┬────┘                     │  (-) ◄──┬───┤  (feedback)                 │
│         │                          │         │   │                             │
│        GND                         └────┬────┘   │                             │
│                                         │        │                             │
│                                         └────────┴─────────► ADC_VREF (3.0V)  │
│                                                                   │            │
│                                                              ┌────┴────┐       │
│                                                              │  22µF   │ C7    │
│                                                              └────┬────┘       │
│                                                              ┌────┴────┐       │
│                                                              │  100nF  │ C7A   │
│                                                              └────┬────┘       │
│                                                                   │            │
│                                                                  GND           │
│                                                                                 │
└────────────────────────────────────────────────────────────────────────────────┘
```

### Why a Buffer?

Without buffer, R7 provides only 0.3mA to share between LM4040 and NTC loads:

- Brew NTC at 93°C: ~441µA
- Steam NTC at 135°C: ~1.36mA
- Total: ~1.8mA (6× more than available!)

**Without buffer:** ADC_VREF collapses to ~0.5V → SYSTEM FAILURE  
**With buffer:** Op-amp drives load from 3.3V rail → ADC_VREF stable at 3.0V

### Component Values

| Component | Value          | Package  | Notes                |
| --------- | -------------- | -------- | -------------------- |
| U5        | LM4040DIM3-3.0 | SOT-23-3 | 3.0V shunt reference |
| U9        | OPA2342UA      | SOIC-8   | Dual RRIO op-amp     |
| R7        | 1kΩ 1%         | 0805     | Bias resistor        |
| R_ISO     | 47Ω 1%         | 0805     | Buffer isolation     |
| C7        | 22µF 10V X5R   | 1206     | Bulk capacitor       |
| C7A       | 100nF 25V      | 0805     | HF decoupling        |

## Power Distribution and Filtering

### Decoupling Capacitor Placement

| Location                 | Capacitor  | Type               | Notes                                  |
| ------------------------ | ---------- | ------------------ | -------------------------------------- |
| 5V rail main             | 470µF      | SMD V-Chip (6.3V)  | Near HLK-15M05C output                 |
| 5V at RP2354 VSYS        | 100nF      | Ceramic (0805)     | Adjacent to pin                        |
| 5V at each relay driver  | 100nF      | Ceramic (0805)     | Suppress switching noise               |
| 3.3V Buck output (U3)    | 2×22µF     | Ceramic (1206)     | For stability                          |
| **3.3V Rail bulk (C5)**  | **47µF**   | **Ceramic (1206)** | **WiFi/relay transient stabilization** |
| **3.3V at RP2354 IOVDD** | **100nF**  | **Ceramic (0805)** | **One per IOVDD pin (decoupling)**     |
| 3.3V at each ADC input   | 100nF      | Ceramic (0603)     | Filter network                         |
| ADC_VREF                 | 22µF+100nF | Ceramic            | Reference stability                    |
| **RP2354 DVDD (1.1V)**   | **1µF**    | **Ceramic (0805)** | **Core voltage bulk**                  |
| **RP2354 DVDD (1.1V)**   | **10µF**   | **Ceramic (1206)** | **Core voltage additional bulk**       |

### 3.3V Rail Bulk Capacitance (ECO-05)

**⚠️ CRITICAL:** The RP2354 discrete chip requires proper decoupling. When driving external loads (ESP32 logic, sensor buffers), additional bulk capacitance is required to prevent brownouts during:

- **WiFi transmission bursts** (ESP32 module)
- **Relay switching transients** (back-EMF coupling)
- **ESP32 current spikes** (up to 500mA during WiFi TX)

**Required Addition:**

- **C5**: 47µF 10V X5R ceramic capacitor (1206 package)
- **Placement**: Adjacent to TPS563200 output, parallel with C4/C4A

```
    TPS563200 Output
         │
    ┌────┴────┐
    │  22µF   │  C4 (existing)
    │   10V   │
    └────┬────┘
         │
    ┌────┴────┐
    │  22µF   │  C4A (existing)
    │   10V   │
    └────┬────┘
         │
    ┌────┴────┐
    │  47µF   │  C5 (NEW - ECO-05)
    │   10V   │  Bulk for transient stability
    └────┬────┘
         │
         └──────────────────► +3.3V Rail
```

**Total 3.3V Rail Capacitance:** 22µF + 22µF + 47µF = **91µF** (adequate for ~1A transient load)

### RP2354 Power Requirements

**Core Voltage (DVDD - 1.1V):**

- Generated internally via VREG (SMPS mode)
- Requires 2.2µH inductor (L2) on VREG_OUT/DVDD path
- Decoupling: 1µF + 10µF capacitors on DVDD rail
- Typical current: 30mA, Peak: 60mA

**I/O Voltage (IOVDD - 3.3V):**

- Supplied from TPS563200 buck converter output
- Decoupling: 100nF ceramic capacitor at each IOVDD pin
- Typical current: 50mA, Peak: 100mA

**Power Sequencing:**

1. 5V rail powers up (from HLK-15M05C)
2. TPS563200 generates 3.3V (IOVDD)
3. RP2354 VREG generates 1.1V (DVDD) from 3.3V input
4. MCU boots and initializes

**⚠️ IMPORTANT:** Ensure IOVDD is present before applying signals to GPIO pins. The RP2354 GPIOs are NOT 5V tolerant and require IOVDD to be powered for safe operation.
