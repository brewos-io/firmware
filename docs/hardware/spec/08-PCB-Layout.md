# PCB Design Requirements

## Board Specifications

| Parameter | Specification |
|-----------|---------------|
| Dimensions | 130mm × 80mm |
| Layers | 2 (Top + Bottom) |
| Material | FR-4 TG130 or higher |
| Thickness | 1.6mm ±10% |
| Copper Weight | 2oz (70µm) both layers |
| Min Trace/Space | 0.2mm / 0.2mm (8mil/8mil) |
| Min Drill | 0.3mm |
| Surface Finish | ENIG (preferred) or HASL Lead-Free |
| Solder Mask | Green (both sides) |
| Silkscreen | White (both sides) |
| IPC Class | Class 2 minimum |

## Mounting Holes

| Hole | Size | Type | Location | Notes |
|------|------|------|----------|-------|
| MH1 | M3 (3.2mm) | PTH | Corner | PE star ground point |
| MH2 | M3 (3.2mm) | NPTH | Corner | Isolated |
| MH3 | M3 (3.2mm) | NPTH | Corner | Isolated |
| MH4 | M3 (3.2mm) | NPTH | Corner | Isolated |

Position all mounting holes 5mm from board edges.

---

## Trace Width Requirements

### High-Voltage Section (220V AC)

| Current | Min Trace Width | Notes |
|---------|-----------------|-------|
| 16A (pump) | 5mm | K2 relay path |
| 5A (general) | 2mm | K1, K3 relay paths |
| Mains L/N | 3mm | Input traces |

### Low-Voltage Section

| Signal Type | Trace Width | Notes |
|-------------|-------------|-------|
| 5V power | 1.0mm | Main distribution |
| 3.3V power | 0.5mm | Logic supply |
| GPIO signals | 0.25mm | Digital I/O |
| ADC signals | 0.3mm | Guarded, short runs |

---

## Layer Stackup (2-Layer)

```
┌─────────────────────────────────────────┐
│  TOP COPPER (2oz)                       │
│  - Signal routing                       │
│  - Component pads                       │
│  - HV traces (wide, isolated)           │
├─────────────────────────────────────────┤
│  FR-4 CORE (1.6mm)                      │
│  TG130 minimum                          │
├─────────────────────────────────────────┤
│  BOTTOM COPPER (2oz)                    │
│  - Ground plane (LV section)            │
│  - Power distribution                   │
│  - Return paths                         │
└─────────────────────────────────────────┘
```

---

## PCB Layout Zones

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              PCB LAYOUT ZONES                                    │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────────────┐ ║ ┌──────────────────────────────────────────┐   │
│  │                          │ ║ │                                          │   │
│  │   HIGH VOLTAGE SECTION   │ ║ │         LOW VOLTAGE SECTION              │   │
│  │                          │ ║ │                                          │   │
│  │  • HLK-15M05C module     │ ║ │  ┌────────────────────────────────────┐ │   │
│  │  • Mains input (J1)      │ ║ │  │  ANALOG SECTION                    │ │   │
│  │  • Fuses (F1, F2)        │ ║ │  │  • ADC VREF (U5, U9)              │ │   │
│  │  • Varistor (RV1)        │ ║ │  │  • NTC inputs (R1, R2)            │ │   │
│  │  • Relays (K1, K2, K3)   │ ║ │  │  • Pressure input                  │ │   │
│  │  • MOVs (RV2, RV3)       │ ║ │  │  • Level probe (U6, U7)           │ │   │
│  │  • 220V spade terminals  │ ║ │  └────────────────────────────────────┘ │   │
│  │                          │ ║ │                                          │   │
│  │  2oz copper minimum      │ ║ │  ┌────────────────────────────────────┐ │   │
│  │  Wide traces (>2mm)      │ ║ │  │  DIGITAL SECTION                   │ │   │
│  │                          │ ║ │  │  • Raspberry Pi Pico 2 (U1)       │ │   │
│  └──────────────────────────┘ ║ │  │  • Transistor drivers (Q1-Q5)     │ │   │
│                               ║ │  │  • LEDs                            │ │   │
│          ISOLATION            ║ │  │  • Connectors (J15-J26)           │ │   │
│          SLOT/GAP             ║ │  └────────────────────────────────────┘ │   │
│          (6mm min)            ║ │                                          │   │
│                               ║ └──────────────────────────────────────────┘   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## Creepage and Clearance

### IEC 60950-1 / IEC 62368-1 Requirements

| Boundary | Isolation | Creepage | Clearance |
|----------|-----------|----------|-----------|
| Mains → LV (5V) | Reinforced | **6.0mm** | **4.0mm** |
| Relay coil → contacts | Basic | 3.0mm | 2.5mm |
| Phase → Neutral | Functional | 2.5mm | 2.5mm |

### Implementation

- **Routed slot** between HV and LV sections (minimum 2mm wide)
- **No copper pour** in isolation zone
- **Conformal coating** recommended for production

---

## Critical Layout Notes

### Analog Section (High Priority)

1. **ADC traces**: Keep short, route away from switching noise
2. **Ground plane**: Solid under analog section, connect at single point
3. **Ferrite bead** (FB1): Place between digital and analog 3.3V
4. **Reference capacitors** (C7, C7A): Place directly at U9 output
5. **NTC filter caps** (C8, C9): Place close to ADC pins

### Power Section

1. **Buck converter**: Tight layout, short SW trace
2. **Input/output caps**: Adjacent to TPS563200
3. **Inductor**: Keep away from sensitive analog

### High-Voltage Section

1. **Wide traces**: ≥5mm for 16A pump path
2. **Thermal relief**: On relay pads for easier soldering
3. **MOV placement**: Close to relay terminals
4. **Slot isolation**: 6mm minimum to LV section

### EMI Considerations

1. **Keep HV traces short** and away from board edges
2. **Ground pour** around sensitive signals
3. **Guard traces** around ADC inputs
4. **Decoupling caps** at every IC VCC pin

---

## Grounding Strategy

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              GROUNDING HIERARCHY                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│                              PE (Chassis Ground)                                │
│                                    │                                            │
│                              ┌─────┴─────┐                                      │
│                              │    MH1    │  ← PE Star Point (PTH mounting hole) │
│                              │  (PTH)    │                                      │
│                              └─────┬─────┘                                      │
│                                    │                                            │
│                    ┌───────────────┴───────────────┐                            │
│                    │                               │                            │
│              ┌─────┴─────┐                   ┌─────┴─────┐                      │
│              │  HV GND   │                   │  LV GND   │                      │
│              │  (Mains)  │                   │  (Logic)  │                      │
│              └─────┬─────┘                   └─────┬─────┘                      │
│                    │                               │                            │
│                 Isolated                    ┌──────┴──────┐                     │
│              via HLK module                 │             │                     │
│                                       ┌─────┴────┐  ┌─────┴────┐               │
│                                       │ DGND     │  │ AGND     │               │
│                                       │ (Digital)│  │ (Analog) │               │
│                                       └──────────┘  └──────────┘               │
│                                                                                  │
│    KEY RULES:                                                                   │
│    ──────────                                                                   │
│    1. Single connection point between PE and LV GND (at MH1)                   │
│    2. HV GND isolated from LV GND via HLK module                               │
│    3. AGND and DGND connect at single point near Pico ADC_GND                  │
│    4. No ground loops - star topology only                                     │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## Silkscreen Requirements

### Mandatory Markings

- Board name and version
- Safety warnings near HV section ("⚠️ HIGH VOLTAGE")
- Pin 1 indicators on all connectors
- Polarity markings (diodes, caps, LEDs)
- Jumper configuration labels (JP1-JP4)
- Test point labels (TP1-TP3)
- Component references

### Recommended

- QR code linking to documentation
- Manufacturer logo/branding
- Date code placeholder

