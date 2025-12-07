# Relay & SSR Output Circuits

## Relay Driver Circuit

All relays use identical driver circuits with integrated indicator LEDs.

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                           RELAY DRIVER CIRCUIT                                  │
│                    (Identical for K1, K2, K3)                                  │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│                                5V Rail                                          │
│                                  │                                              │
│              ┌───────────────────┴───────────────────┐                         │
│              │                                       │                         │
│         ┌────┴────┐                             ┌────┴────┐                    │
│         │  Relay  │                             │  470Ω   │ ← R30+n           │
│         │  Coil   │                             │  (LED)  │   LED resistor    │
│         │  5V DC  │                             └────┬────┘                    │
│         │         │                                  │                         │
│         └────┬────┘                             ┌────┴────┐                    │
│              │                                  │   LED   │                    │
│         ┌────┴────┐                             │  Green  │  ← Indicator      │
│         │  1N4007 │  ← Flyback diode            │  0805   │                    │
│         │   (D)   │    Catches coil spike       └────┬────┘                    │
│         └────┬────┘                                  │                         │
│              │                                       │                         │
│              └───────────────────┬───────────────────┤                         │
│                                  │                   │                         │
│                             ┌────┴────┐              │                         │
│                             │    C    │              │                         │
│                             │ MMBT2222│              │                         │
│                             │  (Q)    │              │                         │
│     GPIO ─────────[470Ω]───►│    B    │              │                         │
│    (10-12)        R20+n     │    E    ├──────────────┘                         │
│              │              └────┬────┘                                        │
│         ┌────┴────┐              │                                             │
│         │  4.7kΩ  │             GND                                            │
│         │R11+n    │  ← Pull-down (relay OFF at boot, RP2350 errata E9)        │
│         └────┬────┘                                                            │
│              │                                                                  │
│             GND                                                                │
│                                                                                 │
└────────────────────────────────────────────────────────────────────────────────┘
```

### Operation

| GPIO State | Transistor | Relay | LED |
| ---------- | ---------- | ----- | --- |
| LOW        | OFF        | OFF   | OFF |
| HIGH       | ON         | ON    | ON  |

### Current Calculations

| Parameter          | Value | Notes                |
| ------------------ | ----- | -------------------- |
| Relay coil (K2)    | ~70mA | 5V / 70Ω             |
| Relay coil (K1/K3) | ~40mA | 5V / 125Ω            |
| LED current        | 6.4mA | (5V - 2.0V) / 470Ω   |
| Base current       | ~6mA  | (3.3V - 0.7V) / 470Ω |

### Relay Selection

| Relay | Part Number          | Rating            | Purpose              | Size     |
| ----- | -------------------- | ----------------- | -------------------- | -------- |
| K1    | Panasonic APAN3105   | 3A @ 250V AC      | Mains indicator lamp | Slim 5mm |
| K2    | Omron G5LE-1A4-E DC5 | **16A** @ 250V AC | Pump motor           | Standard |
| K3    | Panasonic APAN3105   | 3A @ 250V AC      | Solenoid valve       | Slim 5mm |

**⚠️ K2 MUST use -E (high capacity) variant for 16A rating! Standard G5LE-1A4 is only 10A.**

### MOV Surge Protection

For inductive loads (pump, solenoid), MOVs prevent relay contact damage:

```
     Relay Contact (NO)
          │
          ├───────────┐
          │           │
     To  Load     ┌───┴───┐
    (Pump/Valve)  │  MOV  │  RV2/RV3 (275V AC)
                  │ 275V  │  S10K275
                  └───┬───┘
                      │
               Load return (N)
```

| Relay         | MOV | Required?              |
| ------------- | --- | ---------------------- |
| K2 (Pump)     | RV2 | **MANDATORY**          |
| K3 (Solenoid) | RV3 | **MANDATORY**          |
| K1 (Lamp)     | -   | Not needed (resistive) |

---

## SSR Trigger Circuit

External SSRs require 4-30V DC trigger input. Pico's 3.3V GPIO cannot drive SSR directly.

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                           SSR TRIGGER CIRCUIT                                   │
│                    (For SSR1 Brew Heater, SSR2 Steam Heater)                   │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│                                +5V (from J26)                                  │
│                                   │                                            │
│                              ┌────┴────┐                                       │
│                              │   SSR   │                                       │
│                              │   DC+   │  → To external SSR (DC+)              │
│                              │(internal│                                       │
│                              │ ~200Ω)  │                                       │
│                              └────┬────┘                                       │
│                                   │                                            │
│                   ┌───────────────┴───────────────┐                            │
│                   │                               │                            │
│              ┌────┴────┐                     ┌────┴────┐                       │
│              │ Orange  │                     │    C    │                       │
│              │   LED   │                     │  Q5/Q6  │  MMBT2222A            │
│              │  0805   │ Indicator           │   NPN   │                       │
│              └────┬────┘                     │         │                       │
│                   │                          │    B    │◄────[470Ω]────GPIO    │
│              ┌────┴────┐                     │         │     R24/R25   (13/14) │
│              │  330Ω   │  R34/R35            │    E    │                  │    │
│              │         │                     └────┬────┘             ┌───┴───┐│
│              └────┬────┘                          │                  │ 4.7kΩ ││
│                   │                               │                  │R14/R15││
│                   └───────────────┬───────────────┘                  └───┬───┘│
│                                   │                                      │    │
│                                   │                  SSR DC- ◄───────────┤    │
│                                   │                  (to J26)            │    │
│                                  GND                                    GND   │
│                                                                                │
└────────────────────────────────────────────────────────────────────────────────┘
```

### SSR Connection via J26

| Pin | Signal   | Wire  | Connection |
| --- | -------- | ----- | ---------- |
| 15  | +5V      | Red   | SSR1 DC+   |
| 16  | SSR1_NEG | White | SSR1 DC-   |
| 17  | +5V      | Red   | SSR2 DC+   |
| 18  | SSR2_NEG | White | SSR2 DC-   |

### SSR Selection

| Parameter       | Requirement                                     |
| --------------- | ----------------------------------------------- |
| Type            | DC control, AC load (random fire or zero-cross) |
| Rating          | 40A minimum (1400W heater = 6A @ 240V)          |
| Control voltage | 3-32V DC (5V drive is perfect)                  |
| Recommended     | Fotek SSR-40DA or equivalent                    |

---

## Relay Contact Wiring

### J2 - Mains Indicator Lamp (K1)

```
MAINS INPUT                K1 RELAY                    LAMP
    L ─────────────────►│ COM ─────────────────────────► LAMP
                        │     (switched when K1 ON)
                        │ NO ◄─────────────────────────┤
                                                        │
    N ──────────────────────────────────────────────────┘
```

### J3 - Pump (K2)

```
MAINS INPUT                K2 RELAY                    PUMP
    L ─────────────────►│ COM ─────────────────────────► PUMP
                        │        ┌─────────────┐
                        │ NO ◄───┤     RV2     │  MOV 275V
                                 │   S10K275   │
                                 └──────┬──────┘
    N ────────────────────────────────────────────────────┘
```

### J4 - Solenoid Valve (K3)

```
MAINS INPUT                K3 RELAY                  SOLENOID
    L ─────────────────►│ COM ─────────────────────────► VALVE
                        │        ┌─────────────┐
                        │ NO ◄───┤     RV3     │  MOV 275V
                                 │   S10K275   │
                                 └──────┬──────┘
    N ────────────────────────────────────────────────────┘
```
