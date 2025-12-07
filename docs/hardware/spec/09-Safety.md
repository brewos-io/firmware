# Safety & Protection

## Mains Input Protection

### Protection Components

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                         MAINS INPUT PROTECTION                                  │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│    L (Live) ───┬───[F1 10A]───┬──────────────────────────► L_FUSED            │
│                │              │                                                 │
│           ┌────┴────┐    ┌────┴────┐                                           │
│           │   MOV   │    │   X2    │                                           │
│           │  275V   │    │  100nF  │                                           │
│           │  (RV1)  │    │  (C1)   │                                           │
│           └────┬────┘    └────┬────┘                                           │
│                │              │                                                 │
│    N (Neutral) ┴──────────────┴──────────────────────────► N                   │
│                                                                                 │
│    PE (Earth) ───────────────────────────────────────────► Chassis             │
│                                                                                 │
└────────────────────────────────────────────────────────────────────────────────┘
```

### Component Specifications

| Component | Value | Part Number | Purpose |
|-----------|-------|-------------|---------|
| F1 | 10A 250V slow-blow | Littelfuse 0218010.MXP | Main circuit protection |
| F2 | 2A 250V slow-blow | Littelfuse 0218002.MXP | HLK module protection |
| RV1 | 275V AC MOV | S14K275 | Surge protection |
| C1 | 100nF X2 275V AC | - | EMI filter |

### Fuse Coordination

```
Fuse Hierarchy (selectivity):
─────────────────────────────

F1 (10A) protects: Relay loads (pump, solenoid, lamp)
    └─► Trip current: ~15A (slow-blow)
    └─► Trip time: ~10s at 200% load

F2 (2A) protects: HLK-15M05C module only
    └─► Trip current: ~3A
    └─► Trip time: ~1s at 200% load
    └─► Faster than F1 → protects HLK without affecting relays

Machine MCB (16A): House circuit protection
    └─► Trips on dead short only
    └─► F1/F2 clear faults before MCB trips
```

---

## Creepage and Clearance Requirements

### IEC 60950-1 / IEC 62368-1 Compliance

| Boundary | Type | Creepage | Clearance | Implementation |
|----------|------|----------|-----------|----------------|
| Mains → 5V DC | Reinforced | **6.0mm** | **4.0mm** | Routed slot + wide gap |
| Relay coil → contacts | Basic | 3.0mm | 2.5mm | Internal relay design |
| Phase → Neutral | Functional | 2.5mm | 2.5mm | Trace spacing |
| HV GND → LV GND | Reinforced | 6.0mm | 4.0mm | Via HLK isolation |

### PCB Implementation

- **Routed slot** (minimum 2mm wide) between HV and LV sections
- **No copper pour** crosses isolation boundary
- **Silkscreen warning** at HV/LV boundary

---

## ESD and Transient Protection

### Input Protection Summary

| Signal | Protection | Device | Notes |
|--------|------------|--------|-------|
| GPIO2 (Water) | ESD clamp | PESD5V0S1BL (D10) | SOD-323 |
| GPIO3 (Tank) | ESD clamp | PESD5V0S1BL (D11) | SOD-323 |
| GPIO4 (Level) | ESD clamp | PESD5V0S1BL (D12) | SOD-323 |
| GPIO5 (Brew) | ESD clamp | PESD5V0S1BL (D13) | SOD-323 |
| ADC0 (Brew NTC) | ESD clamp | PESD5V0S1BL (D14) | SOD-323 |
| ADC1 (Steam NTC) | ESD clamp | PESD5V0S1BL (D15) | SOD-323 |
| ADC2 (Pressure) | Schottky clamp | BAT54S (D16) | Overvoltage protection |
| 5V Rail | TVS | SMBJ5.0A (D20) | Surge protection |
| RS485 A/B | TVS | SM712 (D21) | Asymmetric (-7V/+12V) |
| Service TX/RX | Zener clamp | BZT52C3V3 (D23/D24) | 5V TTL protection |

### Pressure ADC Protection Detail

```
                 R4 (5.6kΩ)
PRESS_SIG ────────┬──────────────────► GPIO28 (ADC2)
(0.5-4.5V)        │                         │
             ┌────┴────┐              ┌─────┴─────┐
             │  10kΩ   │              │  BAT54S   │
             │   R3    │              │   D16     │
             └────┬────┘              │ Schottky  │
                  │                   └─────┬─────┘
                 GND                       +3.3V
```

**Protection Scenario:** If R3 fails open, full 5V appears at GPIO28.
BAT54S clamps to 3.3V + 0.3V = 3.6V (within RP2350 absolute max).

---

## Thermal Management

### Heat Sources

| Component | Power Dissipation | Mitigation |
|-----------|-------------------|------------|
| HLK-15M05C | ~3W at full load | Adequate PCB area, natural convection |
| TPS563200 | ~25mW | Minimal (>90% efficient) |
| Relay coils | ~0.35W (K2) | Continuous operation OK |
| Transistors | <10mW each | No heatsink needed |

### Design Considerations

1. **HLK module placement:** Near board edge for airflow
2. **No components under HLK:** Heat rises
3. **Thermal relief on pads:** Easier soldering, acceptable for non-power components
4. **Conformal coating:** Protects against moisture in hot/humid environment

---

## Conformal Coating Guidance

### Recommended Areas

| Area | Coating | Notes |
|------|---------|-------|
| HV section (relays, MOVs, fuse) | ✅ Yes | Prevents tracking from contamination |
| Level probe trace area | ✅ Yes | High-impedance, sensitive to moisture |
| LV analog section | ✅ Yes | Protects ADC inputs from drift |
| Connectors (J15, J17, J26) | ❌ Mask off | Must remain solderable |
| Pico module socket | ❌ Mask off | Module must be removable |
| Relay contacts | ❌ Mask off | Internal, already sealed |
| Test points (TP1-TP3) | ❌ Mask off | Must remain accessible |

**Coating Type:** Acrylic (MG Chemicals 419D) or silicone-based.
**Apply:** After all testing complete.

---

## Safety Compliance Notes

### Applicable Standards

| Standard | Scope | Status |
|----------|-------|--------|
| IEC 60335-1 | Household appliances - General | Reference for safety |
| IEC 60950-1 / 62368-1 | IT equipment safety | Creepage/clearance |
| UL 94 V-0 | PCB flammability | FR-4 material |
| CE marking | European conformity | Design target |

### Critical Safety Points

1. **Only relay-switched loads flow through control board** (pump, valves ~6A max)
2. **Heater power does NOT flow through PCB** (external SSRs connect directly to mains)
3. **Power metering HV does NOT flow through PCB** (external modules handle their own mains)
4. **Fused live bus** (10A) feeds relay COMs only
5. **6mm creepage/clearance** required between HV and LV sections
6. **MH1 = PE star point** (PTH), MH2-4 = NPTH (isolated)

### MOV Safety (IEC 60335-1 §19.11.2)

MOVs are placed across **LOADS** (not across relay contacts):

- If MOV shorts → L-N short → fuse clears → safe
- If MOV was across contacts → actuator bypasses control → dangerous

---

## Reliability & MTBF Estimate

### Component MTBF Analysis

| Component       | Estimated MTBF (hours) | Failure Mode                    | Mitigation                          |
|-----------------|------------------------|----------------------------------|-------------------------------------|
| HLK-15M05C      | 100,000                | Electrolytic capacitor aging    | Derated operation, thermal mgmt     |
| Relays (K1-K3)  | 500,000 (mechanical)   | Contact wear, coil burnout      | Relay drivers with flyback diodes   |
| RP2350 Pico     | 1,000,000+             | Overvoltage, ESD damage         | ESD protection on all GPIO          |
| MOV (RV1)       | Degrades with surges   | Open after multiple surges      | Replaceable, visual inspection      |
| Fuses (F1, F2)  | N/A (consumable)       | Normal operation on fault       | Spare fuses provided                |

### System MTBF Estimate

**Estimated System MTBF:** ~50,000 hours (5.7 years) under normal operating conditions

**Assumptions:**
- Operating temperature: 25°C average ambient (derated for higher temps)
- Duty cycle: 2-3 hours/day typical espresso machine usage
- Surge events: <10 per year (typical residential)
- No exposure to extreme humidity or contaminants

**Factors Affecting MTBF:**
- Ambient temperature (MTBF halves for every 10°C above 25°C)
- Voltage surge frequency (MOV degradation)
- Relay switching cycles (pump/valve actuation frequency)

**Recommended Service Interval:** Inspect MOV condition and relay contacts every 3 years or 15,000 operating hours.

---

## Test Points

| TP | Signal | Purpose | Expected Value |
|----|--------|---------|----------------|
| TP1 | GPIO20 | RS485 DE/RE verification | 0V or 3.3V |
| TP2 | ADC_VREF | Reference calibration | 3.00V ±0.5% |
| TP3 | 5V_DIV | 5V monitor / ratiometric cal | ~1.8V |

