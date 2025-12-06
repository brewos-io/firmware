# ECM Synchronika Control Board - Schematic Reference

## Detailed Circuit Schematics for PCB Design

**Board Size:** 130mm × 80mm (2-layer, 2oz copper)  
**Enclosure:** 150mm × 100mm mounting area  
**Revision:** Matches ECM_Control_Board_Specification_v2.md (v2.22)

---

## ⚠️ KEY DESIGN CHANGES IN v2.21+

1. **Universal External Power Metering** - REMOVED embedded PZEM daughterboard
2. **J17 (JST-XH 6-pin)** - LV power meter interface (TTL UART + RS485)
3. **J24 (Screw Terminal 3-pos)** - HV power meter input (L fused, N, PE)
4. **Supports multiple modules:** PZEM-004T, JSY-MK-163T/194T, Eastron SDM, and more
5. **MAX3485 RS485 transceiver (U8)** - On-board with jumper-selectable 120Ω termination
6. **JP2/JP3 NTC jumpers** - Select between 50kΩ (ECM) and 10kΩ (Rocket/Gaggia) NTC sensors
7. **J26 reduced to 22-pos** - CT clamp pins removed (now on external module)
8. **No HV on PCB for metering** - External modules handle their own mains connections
9. **GPIO20 → RS485 DE/RE** - Direction control for industrial meters
10. **Unified J26 Screw Terminal (22-pos)** - Switches, sensors, SSR outputs (no CT)
11. **6.3mm spades retained ONLY for 220V AC**: Mains input (L, N, PE), relay outputs
12. **OPA342 + TLV3201** for steam boiler level probe (AC sensing)
13. **HLK-15M05C** power supply (3A/15W, 48×28×18mm)
14. **MOV arc suppression MANDATORY** for K2 (Pump) and K3 (Solenoid)
15. **Mounting holes**: MH1=PE star point (PTH), MH2-4=NPTH (isolated)

---

# Sheet 1: Power Supply

## 1.1 Mains Input & Protection

```
                                    MAINS INPUT & PROTECTION
    ════════════════════════════════════════════════════════════════════════════

    J1 (Mains Input)
    ┌─────────────┐
    │ L  (Live)   ├────┬──────────────────────────────────────────────────────────►
    │             │    │
    │ N  (Neutral)├──┐ │    F1                RV1              C1
    │             │  │ │  ┌─────┐          ┌──────┐        ┌──────┐
    │ PE (Earth)  ├─┐│ └──┤ 10A ├────┬─────┤ MOV  ├────┬───┤ 100nF├───┬─────► L_FUSED
    └─────────────┘ ││    │250V │    │     │ 275V │    │   │ X2   │   │
                    ││    │T/Lag│    │     │ 14mm │    │   │275VAC│   │
        To Chassis  ││    └─────┘    │     └──┬───┘    │   └──┬───┘   │
        Ground      ││               │        │        │      │       │
        (if metal   │└───────────────┼────────┴────────┴──────┴───────┴─────► N_FUSED
        enclosure)  │                │
                    │                │
                   ─┴─              ─┴─
                   GND              GND
                   (PE)            (Mains)

    Component Values:
    ─────────────────
    F1:  Littelfuse 0218010.MXP, 10A 250V, 5x20mm, Slow-blow (relay loads only ~6A)
    RV1: Epcos S14K275 or Littelfuse V275LA20AP, 275VAC, 14mm disc
    C1:  TDK B32922C3104M, 100nF, 275VAC, X2 Safety Rated
```

## 1.2 AC/DC Isolated Power Supply

```
                                AC/DC ISOLATED CONVERTER
    ════════════════════════════════════════════════════════════════════════════

                                    U2: Hi-Link HLK-15M05C
                                    (3A, compact 16mm height)
                              ┌──────────────────────────┐
                              │                          │
    L_FUSED ──────────────────┤ L               +Vout    ├───────┬──────────► +5V
                              │     ╔═══════╗            │       │
                              │     ║ 3kV   ║            │   C2  │
                              │     ║ISOLATE║            │ ┌─────┴─────┐
    N_FUSED ──────────────────┤ N   ╚═══════╝    -Vout   │ │   100µF   │
                              │                          ├─┤   16V     │
                              │                          │ │   Alum.   │
                              └──────────────────────────┘ └─────┬─────┘
                                                                 │
                                                                ─┴─
                                                                GND
                                                              (Secondary)

    NOTES:
    ══════
    • Primary side (L, N) is MAINS VOLTAGE - maintain 6mm clearance to secondary
    • Secondary side (-Vout) becomes system GND
    • GND is ISOLATED from mains PE (earth)
    • HLK-15M05C is 48×28×18mm - verify clearance to fuse holder

    Component Values:
    ─────────────────
    U2:  Hi-Link HLK-15M05C (5V 3A/15W) - adequate for ~1.1A peak load
         Alt: Mean Well IRM-20-5 (5V 4A) if more headroom needed
    C2:  470µF 6.3V Polymer (low ESR, long life in hot environment)
```

## 1.3 5V to 3.3V Synchronous Buck Converter (v2.22 Update)

**⚠️ v2.22 CHANGE:** Replaced LDO (AP2112K) with synchronous buck converter (TPS563200)
to eliminate thermal risk identified in engineering review.

```
                            3.3V SYNCHRONOUS BUCK CONVERTER
    ════════════════════════════════════════════════════════════════════════════

                            U3: TPS563200DDCR
                           ┌──────────────────────┐
                           │                      │
    +5V ─────┬─────────────┤ VIN            SW    ├────┬────[L1 4.7µH]────┬──► +3.3V
             │             │                      │    │                   │
         C3  │             │ EN              FB   ├────┼───────────────────┤
       ┌─────┴─────┐       │        │             │    │                   │
       │   22µF    │       │        ▼             │    │              ┌────┴────┐
       │   25V     │       │       GND            │    │              │  22µF   │
       │ Ceramic   │       └──────────────────────┘    │              │  10V    │
       │  (X5R)    │                                   │              │  (C4)   │
       └─────┬─────┘                                   │              └────┬────┘
             │                                         │                   │
            ─┴─                                        │              ┌────┴────┐
            GND                                        │              │  22µF   │
                                                       │              │  10V    │
                                                       │              │  (C4A)  │
                                                       │              └────┬────┘
                                                      ─┴─                 ─┴─
                                                      GND                 GND

    ✅ WHY BUCK INSTEAD OF LDO? (Engineering Review Fix)
    ─────────────────────────────────────────────────────
    LDO Problem: P = (5V - 3.3V) × I = 1.7V × I
    At 250mA in hot environment: junction temp exceeds 125°C max
    
    Buck Solution: >90% efficient, minimal self-heating
    Same footprint, better thermal reliability

    Component Values:
    ─────────────────
    U3:  TI TPS563200DDCR, 3A sync buck, SOT-23-6
    L1:  4.7µH, 3A saturation, DCR<100mΩ (Murata LQH32CN4R7M23)
    C3:  22µF 25V X5R Ceramic, 1206 (input)
    C4:  22µF 10V X5R Ceramic, 1206 (output)
    C4A: 22µF 10V X5R Ceramic, 1206 (output, parallel for ripple)
```

## 1.4 Precision ADC Voltage Reference (v2.22 Addition)

**Purpose:** Provides stable reference for NTC measurements, eliminating supply drift errors.

```
                            PRECISION ADC VOLTAGE REFERENCE
    ════════════════════════════════════════════════════════════════════════════

    +3.3V
       │
    ┌──┴──┐
    │ 1kΩ │  R7 - Bias resistor
    │ 1%  │  I = (3.3V - 3.0V) / 1kΩ = 0.3mA
    └──┬──┘
       │
       ├────────────────────────┬────────────────────────────► ADC_VREF (3.0V)
       │                        │                                    │
    ┌──┴──┐                ┌────┴────┐                          ┌────┴────┐
    │LM4040│               │  22µF   │  C7                      │  100nF  │ C7A
    │ 3.0V │  U5           │  10V    │                          │  25V    │
    │ Ref  │               │ Ceramic │                          │ Ceramic │
    └──┬──┘                └────┬────┘                          └────┬────┘
       │                        │                                    │
      ─┴─                      ─┴─                                  ─┴─
      GND                      GND                                  GND


                        Ferrite Bead for ADC Reference
                        ───────────────────────────────

    +3.3V ──────────────[FB1: 600Ω @ 100MHz]──────────► to R7 (above)

    This isolates the precision reference from digital switching noise.

    Component Values:
    ─────────────────
    U5:  TI LM4040DIM3-3.0, 3.0V shunt ref, SOT-23-3
    R7:  1kΩ 1%, 0805 (bias resistor)
    FB1: Murata BLM18PG601SN1D, 600Ω @ 100MHz, 0603
    C7:  22µF 10V X5R Ceramic, 1206 (bulk)
    C7A: 100nF 25V Ceramic, 0805 (HF decoupling)

    Connection to NTC Pull-ups:
    ──────────────────────────
    R1 (Brew NTC) and R2 (Steam NTC) connect to ADC_VREF (3.0V)
    instead of 3.3V rail. This eliminates supply variations.

    Test Point: TP2 provides access to ADC_VREF for verification.
```

---

# Sheet 2: Microcontroller (Raspberry Pi Pico 2)

## 2.1 Pico 2 Module & Decoupling

```
                           RASPBERRY PI PICO 2 MODULE
    ════════════════════════════════════════════════════════════════════════════

                               U1: Raspberry Pi Pico 2

         +3.3V ◄────────────────┐           ┌────────────────► (Internal 3.3V)
                                │           │
         +5V ───────────────────┤ VSYS  3V3 ├───────────────── (3.3V out, max 300mA)
                   ┌────┐       │           │       ┌────┐
         GND ──┬───┤100n├───────┤ GND   GND ├───────┤100n├───┬── GND
               │   └────┘       │           │       └────┘   │
               │                │           │                │
               │     ┌──────────┤ GP0   GP28├────────────────┼─► ADC2 (Pressure)
               │     │          │           │                │
    ESP32_TX◄──┼─────┤          │ GP1   GP27├────────────────┼─► ADC1 (Steam NTC)
               │     │          │           │                │
    ESP32_RX ──┼─────┤          │ GP2   GP26├────────────────┼─► ADC0 (Brew NTC)
               │     │          │           │                │
               │     │          │ GP3   RUN ├────────────────┼── SW1 (Reset)
               │     │          │           │                │
               │     │          │ GP4  GP22 ├────────────────┼── (Spare)
               │     │          │           │                │
               │     │          │ GP5  GP21 ├────────────────┼── (Spare)
               │     │          │           │                │
               │     │          │ GP6  GP20 ├────────────────┼─► (spare)
               │     │          │           │                │
               │     │          │ GP7  GP19 ├────────────────┼─► BUZZER
               │     │          │           │                │
               │     │          │ GP8  GP18 ├────────────────┼─► SPI_SCK
               │     │          │           │                │
               │     │          │ GP9  GP17 ├────────────────┼─► SPI_CS
               │     │          │           │                │
               │     │          │ GP10 GP16 ├────────────────┼─► SPI_MISO
               │     │          │           │                │
               │     │          │ GP11 GP15 ├────────────────┼─► STATUS_LED
               │     │          │           │                │
               │     │          │ GP12 GP14 ├────────────────┼─► SSR2_STEAM
               │     │          │           │                │
               │     │          │ GP13 BOOT ├────────────────┼── SW2 (Bootsel)
               │                │           │                │
               │                │ GND   GND │                │
               │                │     │     │                │
               └────────────────┴─────┼─────┴────────────────┘
                                      │
                                     ─┴─
                                     GND

    Decoupling Capacitors (place adjacent to Pico):
    ───────────────────────────────────────────────
    • 100nF ceramic on each VSYS pin
    • 100nF ceramic on each 3V3 pin
    • 10µF ceramic near VSYS input (optional)
```

## 2.2 Reset and Boot Buttons

```
                            RESET AND BOOT BUTTONS
    ════════════════════════════════════════════════════════════════════════════

    RESET BUTTON (SW1)                      BOOT BUTTON (SW2)
    ──────────────────                      ─────────────────

         +3.3V                                    BOOTSEL
           │                                    (Pico TP6)
      ┌────┴────┐                                   │
      │  10kΩ   │  ◄── Internal on Pico            │
      │  R70    │      (may not need external)     │
      └────┬────┘                              ┌───┴───┐
           │                                   │  SW2  │
           ├──────────────► RUN Pin            │ BOOT  │  6x6mm SMD
           │                (Pico)             │       │  Tactile
       ┌───┴───┐                               └───┬───┘
       │  SW1  │  6x6mm SMD                        │
       │ RESET │  Tactile                         ─┴─
       │       │  (EVQP7A01P)                     GND
       └───┬───┘
           │
          ─┴─
          GND

    Operation:
    ──────────
    • Press SW1 → Reset Pico
    • Hold SW2 + Press SW1 + Release SW1 + Release SW2 → Enter USB Bootloader
```

---

# Sheet 3: Relay Drivers

## 3.1 Relay Driver Circuit (K1, K2, K3)

```
                            RELAY DRIVER CIRCUIT
    ════════════════════════════════════════════════════════════════════════════

    (Identical driver circuit for K1, K2, K3. Different relay sizes by load.)

                                        +5V
                                         │
               ┌─────────────────────────┴─────────────────────────┐
               │                                                   │
          ┌────┴────┐                                         ┌────┴────┐
          │  Relay  │                                         │  470Ω   │
          │  Coil   │                                         │  R30+n  │
          │   5V    │                                         │  (LED)  │
          │  K1/2/3 │                                         └────┬────┘
          └────┬────┘                                              │
               │                                              ┌────┴────┐
          ┌────┴────┐                                         │   LED   │
          │  D1/2/3 │ ◄── Flyback diode                       │  Green  │
          │ 1N4007  │                                         │ LED1-3  │
          └────┬────┘                                         └────┬────┘
               │                                                   │
               └───────────────────────┬───────────────────────────┤
                                       │                           │
                                  ┌────┴────┐                      │
                                  │    C    │                      │
                                  │  Q1/2/3 │  MMBT2222A           │
                                  │   NPN   │  SOT-23              │
    GPIO (10/11/12) ────[1kΩ]────►│    B    │                      │
                        R20+n     │    E    ├──────────────────────┘
                           │      └────┬────┘
                      ┌────┴────┐      │
                      │  10kΩ   │     ─┴─
                      │  R10+n  │     GND
                      │ Pull-dn │
                      └────┬────┘
                          ─┴─
                          GND

    OPERATION:
    ──────────
    GPIO LOW  → Transistor OFF → Relay OFF, LED OFF
    GPIO HIGH → Transistor ON  → Relay ON, LED ON

    Relay coil current: ~70mA (through transistor collector)
    LED current: (5V - 2.0V) / 470Ω = 6.4mA (bright indicator)

    Relay Contact Connections (All 220V AC - 6.3mm Spade Terminals):
    ─────────────────────────────────────────────────────────────────

    K1 (Water LED):            K2 (Pump):                K3 (Solenoid):
    J2-NO ── K1-NO             J3-NO ── K2-NO            J4-NO ── K3-NO
    (6.3mm spade, 220V)        (6.3mm spade, 220V 5A)    (6.3mm spade, 220V)
    COM internal to L_FUSED    COM internal to L_FUSED   COM internal to L_FUSED

    Component Values:
    ─────────────────
    K1, K3: Panasonic APAN3105 (5V coil, 3A contacts, slim 5mm, IEC61010)
            • K1 (Indicator Lamp): ~100mA load, 3A relay is plenty
              ⚠️ v2.22 CLARIFICATION: Switches mains indicator lamp (low current)
            • K3 (Solenoid): ~0.5A load, 3A rating is plenty
    K2:     Omron G5LE-1A4 DC5 (5V coil, 16A contacts, standard size)
            • Pump motor needs robust contacts for inrush current
    D1-D3:  UF4007 (1A, 1000V, 75ns fast recovery) - DO-41
            ⚠️ v2.22 UPGRADE: Fast recovery for snappier relay contact opening
            • Standard 1N4007 causes slow coil decay → "lazy" contact opening
            • UF4007 faster recovery reduces arc time at relay contacts
    Q1-Q3:  MMBT2222A (SOT-23)
    LED1-3: Green 0805, Vf~2.0V
    R20-22: 1kΩ 5% 0805 (transistor base)
    R30-32: 470Ω 5% 0805 (LED current limit - gives 6.4mA)
    R10-12: 10kΩ 5% 0805 (pull-down, ensures relay off at boot)

    ═══════════════════════════════════════════════════════════════════════════
    MOV ARC SUPPRESSION (Inductive Load Protection)
    ═══════════════════════════════════════════════════════════════════════════

    ⚠️ MANDATORY for K2 (Pump) and K3 (Solenoid) - prevents contact welding!

    MOV across relay contacts (HV side):
    ─────────────────────────────────────

         Relay NO ──────┬───────────────────────────► To Load
                        │
                   ┌────┴────┐
                   │   MOV   │  RV2 or RV3 (275V Varistor)
                   │  275VAC │  10mm disc, ~4mm thick
                   │  10mm   │
                   └────┬────┘
                        │
         Relay COM ─────┴───────────────────────────── From L_FUSED

    MOV vs RC Snubber:
    ───────────────────
    • ~70% smaller than X2 capacitor + resistor combo
    • No series resistor needed - simpler BOM
    • Faster clamping response (nanoseconds)
    • Critical with downsized K1/K3 relays (3A contacts)

    MOV Components:
    ────────────────
    K2 (Pump):     RV2 (S10K275) - MANDATORY
    K3 (Solenoid): RV3 (S10K275) - MANDATORY
    K1 (LED):      DNP (footprint only, resistive load)

    MOV Component Values:
    ─────────────────────
    RV2-RV3: S10K275 (275V AC, 10mm disc, 2500A surge)
             Footprint: Disc_D10mm_W5.0mm_P7.50mm
```

---

# Sheet 4: SSR Drivers

## 4.1 SSR Trigger Circuit

```
                            SSR TRIGGER CIRCUIT
    ════════════════════════════════════════════════════════════════════════════

    (Identical circuit for SSR1 and SSR2)

    IMPORTANT: SSR has internal current limiting - NO series resistor needed!
    SSR input spec: 4-32V DC (KS15 D-24Z25-LQ)

                                        +5V
                                         │
               ┌─────────────────────────┴─────────────────────────┐
               │                                                   │
               │                                              ┌────┴────┐
               │                                              │   1kΩ   │
               │                                              │ R34/35  │
               │                                              └────┬────┘
               ▼                                                   │
    ┌──────────────────┐                                      ┌────┴────┐
    │  To SSR Input    │                                      │   LED   │
    │  (+) Positive    │                                      │ Orange  │
    │ J26-19/21 (SSR+) │                                      │ LED5/6  │
    └────────┬─────────┘                                      └────┬────┘
             │                                                     │
             │    ┌───────────────────────────────┐                │
             │    │   EXTERNAL SSR                │                │
             │    │   (has internal LED+resistor) │                │
             │    │   KS15 D-24Z25-LQ             │                │
             │    └───────────────────────────────┘                │
             │                                                     │
    ┌────────┴─────────┐                                           │
    │  To SSR Input    │                                           │
    │  (-) Negative    ├───────────────────────────────────────────┤
    │ J26-20/22 (SSR-) │                                           │
    └────────┬─────────┘                                      ┌────┴────┐
             │                                                │    C    │
             │                                                │  Q5/Q6  │  MMBT2222A
             │                                                │   NPN   │
             └───────────────────────────────────────────────►│    E    │
                                                              └────┬────┘
                                                                   │
    GPIO (13/14) ───────[1kΩ R24/25]──────────────────────────────►B
                                  │                                │
                             ┌────┴────┐                          ─┴─
                             │  10kΩ   │                          GND
                             │ R14/15  │
                             └────┬────┘
                                 ─┴─
                                 GND

    OPERATION:
    ──────────
    GPIO LOW  → Transistor OFF → SSR- floating → SSR OFF (no current path)
    GPIO HIGH → Transistor ON  → SSR- to GND   → SSR ON  (~4.8V across SSR)

    Voltage at SSR = 5V - Vce(sat) = 5V - 0.2V = 4.8V  ✓ (exceeds 4V minimum)

    External SSR Connections:
    ─────────────────────────
    SSR1 (Brew Heater):
        J26-19 (SSR1+) ── SSR Input (+) ── from 5V rail
        J26-20 (SSR1-) ── SSR Input (-) ── to transistor collector
        SSR AC Load side: Connected via EXISTING MACHINE WIRING (not through PCB)

    SSR2 (Steam Heater):
        J26-21 (SSR2+) ── SSR Input (+) ── from 5V rail
        J26-22 (SSR2-) ── SSR Input (-) ── to transistor collector
        SSR AC Load side: Connected via EXISTING MACHINE WIRING (not through PCB)

    ⚠️ IMPORTANT: NO HIGH CURRENT THROUGH PCB
    ────────────────────────────────────────
    The PCB provides ONLY 5V control signals to the SSRs.
    Mains power to SSRs uses the machine's existing wiring:
    • Machine mains Live → SSR AC input
    • SSR AC output → Heater element
    • Heater return → Machine mains Neutral

    Component Values:
    ─────────────────
    External SSRs: KS15 D-24Z25-LQ (25A, 4-32V DC input, 24-280V AC output)
    R24-25: 1kΩ 5% 0805 (transistor base drive, ~3mA)
    R34-35: 1kΩ 5% 0805 (LED current limit, ~3mA)
    R14-15: 10kΩ 5% 0805 (pull-down, keeps SSR OFF at boot)
    Q5-Q6:  MMBT2222A (SOT-23), Vce(sat) < 0.3V @ 100mA
    LED5-6: Orange 0805, Vf~2.0V
```

---

# Sheet 5: Sensor Inputs

## 5.1 NTC Thermistor Inputs

**Multi-Machine Compatibility:** Use **solder jumpers JP2/JP3** to switch NTC configurations.

| Machine Brand     | NTC @ 25°C | JP2       | JP3       | Effective R1 | Effective R2 |
| ----------------- | ---------- | --------- | --------- | ------------ | ------------ |
| **ECM, Profitec** | 50kΩ       | **OPEN**  | **OPEN**  | 3.3kΩ        | 1.2kΩ        |
| Rocket, Rancilio  | 10kΩ       | **CLOSE** | **CLOSE** | ~1kΩ         | ~430Ω        |
| Gaggia            | 10kΩ       | **CLOSE** | **CLOSE** | ~1kΩ         | ~430Ω        |

```
                        NTC THERMISTOR INPUT CIRCUITS
    ════════════════════════════════════════════════════════════════════════════

    ⚠️ EACH SENSOR HAS DIFFERENT PULL-UP - OPTIMIZED FOR TARGET TEMPERATURE
    ⚠️ JP2/JP3 SOLDER JUMPERS SELECT NTC TYPE (NO RESISTOR SWAPPING NEEDED)

    BREW NTC (GPIO26/ADC0)                  STEAM NTC (GPIO27/ADC1)
    Target: 90-100°C                        Target: 125-150°C

         +3.3V_ANALOG                            +3.3V_ANALOG
              │                                       │
         ┌────┴────┐                             ┌────┴────┐
         │  3.3kΩ  │  R1 (Brew)                  │  1.2kΩ  │  R2 (Steam)
         │  ±1%    │  Optimized for 93°C         │  ±1%    │  Optimized for 135°C
         └────┬────┘                             └────┬────┘
              │                                       │
   J26-8 ─────┼────┬───────► GPIO26          J26-10 ──┼────┬───────► GPIO27
              │    │         (ADC0)                   │    │         (ADC1)
         ┌────┴────┐  ┌────┴────┐             ┌────┴────┐  ┌────┴────┐
         │   NTC   │  │  1kΩ    │             │   NTC   │  │  1kΩ    │
         │  50kΩ   │  │  R5     │             │  50kΩ   │  │  R6     │
         │  @25°C  │  └────┬────┘             │  @25°C  │  └────┬────┘
         └────┬────┘       │                  └────┬────┘       │
              │       ┌────┴────┐                  │       ┌────┴────┐
              │       │  100nF  │                  │       │  100nF  │
              │       │   C8    │                  │       │   C9    │
              │       └────┬────┘                  │       └────┬────┘
              │            │                       │            │
              └────────────┴─── GND                └────────────┴─── GND

    ESD Protection (both channels):
    ────────────────────────────────
    J26-8/10 (NTC) ┬──────────────────────────────────────────► To ADC
                   │
              ┌────┴────┐
              │ D10/D11 │  PESD5V0S1BL (ESD clamp)
              └────┬────┘
                  ─┴─
                  GND

    Component Values:
    ─────────────────
    R1:      3.3kΩ ±1%, 0805 (Brew NTC pull-up, always populated)
    R1A:     1.5kΩ ±1%, 0805 (Brew parallel, enabled by JP2)
    R2:      1.2kΩ ±1%, 0805 (Steam NTC pull-up, always populated)
    R2A:     680Ω ±1%, 0805 (Steam parallel, enabled by JP3)
    R5-R6:   1kΩ 1%, 0805 (series protection)
    C8-C9:   100nF 25V, 0805, Ceramic
    D10-D11: PESD5V0S1BL, SOD-323 (bidirectional ESD clamp)
    JP2:     Solder jumper (OPEN=50kΩ NTC, CLOSE=10kΩ NTC)
    JP3:     Solder jumper (OPEN=50kΩ NTC, CLOSE=10kΩ NTC)

    JUMPER MATH:
    • JP2 CLOSED: R1 || R1A = 3.3kΩ || 1.5kΩ ≈ 1.03kΩ (for 10kΩ NTC)
    • JP3 CLOSED: R2 || R2A = 1.2kΩ || 680Ω ≈ 434Ω (for 10kΩ NTC)

    Default: JP2/JP3 OPEN = ECM/Profitec (50kΩ NTC)
    For Rocket/Gaggia: Close JP2 and JP3 with solder bridge

    Resolution at Target Temps:
    ───────────────────────────
    • Brew (93°C):  ~31 ADC counts/°C → 0.032°C resolution
    • Steam (135°C): ~25 ADC counts/°C → 0.04°C resolution
```

## 5.2 K-Type Thermocouple Input

**⚠️ SENSOR RESTRICTION:** MAX31855**K** is hard-wired for **Type-K ONLY**.

- ✅ Type-K (Chromel/Alumel) - Standard for E61 group head thermometers
- ❌ Type-J, Type-T, PT100/RTD (will NOT work - different chips required)

```
                        K-TYPE THERMOCOUPLE INPUT
    ════════════════════════════════════════════════════════════════════════════

                                    +3.3V
                                      │
                                 ┌────┴────┐
                                 │  100nF  │  Decoupling
                                 │   C10   │
                                 └────┬────┘
                                      │
                      U4: MAX31855KASA+
                     ┌─────────────────────────┐
                     │                         │
    J26-12 (TC+)─────┤ T+   (1)        VCC (8) ├───────────────────── +3.3V
    (TC+)            │                         │
                     │                         │
    J26-13 (TC-)─┬───┤ T-   (2)        SCK (7) ├───────────────────── GPIO18 (SPI_SCK)
    (TC-)        │   │                         │
                 │   │                         │
            ┌────┴───┤ NC   (3)         CS (6) ├───────────────────── GPIO17 (SPI_CS)
            │ 10nF   │                         │
            │ C40    │                         │
            └───┬────┤ GND  (4)         DO (5) ├───────────────────── GPIO16 (SPI_MISO)
                │    │                         │
                │    └─────────────────────────┘
                │
               ─┴─
               GND

    Thermocouple Connector:
    ───────────────────────
    J26-12/13: Thermocouple connections (part of unified 24-pos terminal)
    Pin 1: TC+ (Yellow for K-type, or check thermocouple wire color code)
    Pin 2: TC- (Red for K-type)

    NOTE: Use proper K-type thermocouple connectors if available

    Component Values:
    ─────────────────
    U4:  MAX31855KASA+, SOIC-8, K-type specific
    C40: 10nF 50V Ceramic, 0805 (differential noise filter)
    C10: 100nF 25V Ceramic, 0805 (VCC decoupling)

    PCB Layout Notes:
    ─────────────────
    • Keep T+ and T- traces short and symmetric
    • Route away from power traces and relay coils
    • Add ground guard ring around thermocouple traces
```

## 5.3 Pressure Transducer Input (J26 Pin 14-16 - Amplified 0.5-4.5V)

**⚠️ SENSOR RESTRICTION:** Circuit designed for **0.5-4.5V ratiometric ONLY**.

- ✅ 3-wire sensors (5V, GND, Signal) like YD4060 or automotive pressure sensors
- ✅ 0.5V offset = broken wire detection (0.0V = fault, 0.5V = 0 bar)
- ❌ 4-20mA current loop sensors (require different circuit)

```
                    PRESSURE TRANSDUCER INPUT (AMPLIFIED TYPE)
    ════════════════════════════════════════════════════════════════════════════

    J26-14/15/16: Pressure transducer (part of unified 24-pos terminal)

                            +5V
                             │
    J26-14 (P-5V)────────────┴────────────────────────────────── Transducer VCC
    (5V)                                                         (Red wire)

    J26-15 (P-GND)─────────────────────────────────────────────── Transducer GND
    (GND)           │                                            (Black wire)
                   ─┴─
                   GND

    J26-16 (P-SIG)──────────────────────┬─────────────────────── Transducer Signal
    (Signal)                            │                        (Yellow/White wire)
    0.5V - 4.5V                         │
                                   ┌────┴────┐
                                   │  4.7kΩ  │  R4 (Series) ±1%
                                   │         │
                                   └────┬────┘
                                        │
                                        ├──────────────┬───────────────────► GPIO28
                                        │              │                     (ADC2)
                                   ┌────┴────┐    ┌────┴────┐
                                   │  10kΩ   │    │  100nF  │  LP Filter
                                   │   R3    │    │   C11   │
                                   │   ±1%   │    └────┬────┘
                                   └────┬────┘         │
                                        │              │
                                       ─┴─            ─┴─
                                       GND            GND

    Voltage Divider Calculation (OPTIMIZED for 91% ADC range):
    ───────────────────────────────────────────────────────────
    Ratio = R3 / (R3 + R4) = 10k / (10k + 4.7k) = 0.68

    Input 0.5V  → Output 0.34V → ADC ~422
    Input 2.5V  → Output 1.70V → ADC ~2109
    Input 4.5V  → Output 3.06V → ADC ~3795

    Resolution: 0.0047 bar/count (15% better than old 15kΩ design)

    Component Values:
    ─────────────────
    R3: 10kΩ ±1%, 0805 (to GND)
    R4: 4.7kΩ ±1%, 0805 (series, from signal)
    C11: 100nF 25V Ceramic, 0805

    Selected Transducer: YD4060 Series
    ───────────────────────────────────
    - Pressure Range: 0-1.6 MPa (0-16 bar)
    - Output: 0.5-4.5V ratiometric
    - Supply: 5VDC, ≤3mA
    - Thread: 1/8" NPT (use adapter for espresso machine)
    - Wiring: Red=5V, Black=GND, Yellow=Signal
```

## 5.4 Digital Switch Inputs (J26 - Low Voltage Only)

```
                        DIGITAL SWITCH INPUTS
    ════════════════════════════════════════════════════════════════════════════

    All LOW VOLTAGE digital switch inputs are consolidated in J26 (8-position screw terminal).
    (Water Reservoir, Tank Level, Level Probe, Brew Handle - ALL 3.3V LOGIC)

    ⚠️ J26 is for LOW VOLTAGE (3.3V) inputs ONLY!
    ⚠️ 220V relay outputs (K1, K2, K3) use 6.3mm spade terminals (J2, J3, J4)

    J26 LOW-VOLTAGE SWITCH INPUT TERMINAL BLOCK (Phoenix MKDS 1/8-5.08):
    ─────────────────────────────────────────────────────────────────────
    ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
    │  1  │  2  │  3  │  4  │  5  │  6  │  7  │  8  │
    │ S1  │S1-G │ S2  │S2-G │ S3  │ S4  │S4-G │ GND │
    └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
     WtrRes GND  Tank  GND  Probe Brew  GND  Spare

    SWITCH INPUT CIRCUIT (S1, S2, S4):
    ───────────────────────────────────

                            +3.3V
                              │
                         ┌────┴────┐
                         │  10kΩ   │  Pull-up
                         │  R16-18 │  (or use Pico internal)
                         └────┬────┘
                              │
    J26 Pin 1/3/6  ───────────┼───────────────────────┬──────────────► GPIO 2/3/5
    (Switch Signal)           │                       │
                              │                  ┌────┴────┐
                         ┌────┴────┐             │  D12-14 │  ESD
                         │ Switch  │             │  PESD   │  Clamp
                         │  N.O.   │             └────┬────┘
                         └────┬────┘                  │
                              │                       │
    J26 Pin 2/4/7  ───────────┴───────────────────────┴──────────────── GND
    (Switch GND)

    J26 SIGNAL MAPPING (8-pin):
    ───────────────────────────
    Pin 1 (S1)  → GPIO2 (Water Reservoir Switch Signal)
    Pin 2 (S1-G)→ GND   (Water Reservoir Switch Return)
    Pin 3 (S2)  → GPIO3 (Tank Level Sensor Signal)
    Pin 4 (S2-G)→ GND   (Tank Level Sensor Return)
    Pin 5 (S3)  → GPIO4 (Steam Boiler Level Probe - via comparator)
    Pin 6 (S4)  → GPIO5 (Brew Handle Switch Signal)
    Pin 7 (S4-G)→ GND   (Brew Handle Switch Return)
    Pin 8 (GND) → GND   (Common Ground - spare terminal)


    Steam Boiler Level Probe (OPA342 + TLV3201 AC Sensing):
    ─────────────────────────────────────────────────────────

    ⚠️  AC sensing prevents electrolysis and probe corrosion!

    STAGE 1: WIEN BRIDGE OSCILLATOR (~160Hz)
    ────────────────────────────────────────
                          +3.3V
                            │
                       ┌────┴────┐
                       │  100nF  │  C60 (OPA342 decoupling)
                       └────┬────┘
                            │
                     VCC────┤
                            │ U6: OPA342
    ┌────[R91 10kΩ]─────────┤-  (Op-Amp)  ├──┬───────────► AC_OUT
    │                  ┌────┤+             │  │              (to probe)
    │                  │    │     GND      │  │
    │                  │    └──────┬───────┘  │
    │                  │           │          │
    │                  │          ─┴─         │
    │                  │          GND         │
    │                  │                      │
    │             ┌────┴────┐           ┌────┴────┐
    │             │  10kΩ   │           │  10kΩ   │
    │             │  R92    │           │  R93    │
    │             └────┬────┘           └────┬────┘
    │                  │                     │
    │                  ├─────────────────────┤
    │                  │                     │
    │             ┌────┴────┐           ┌────┴────┐
    │             │  100nF  │           │  100nF  │
    │             │  C61    │           │  C62    │
    │             └────┬────┘           └────┬────┘
    │                  │                     │
    │                 ─┴─                   ─┴─
    │                 GND                   GND
    │
    └─────────────────────────────────────────────────────────────────────────┐
                                                                              │
    STAGE 2: PROBE & SIGNAL CONDITIONING                                     │
    ────────────────────────────────────                                      │
                                                                              │
    AC_OUT ───[100Ω R94]───┬────────────────────► J26 Pin 5 (Level Probe S3) │
           (current limit) │                      Screw terminal (LV)         │
                           │                           │                      │
                      ┌────┴────┐                 ┌────┴────┐                 │
                      │   1µF   │                 │  Probe  │                 │
                      │   C64   │                 │   ~~~   │                 │
                      │(coupling)│                 │  Water  │                 │
                      └────┬────┘                 │  Level  │                 │
                           │                      └────┬────┘                 │
                           ▼                           │                      │
                       AC_SENSE                   Boiler Body                 │
                           │                      (Ground via PE)             │
                           │                          ─┴─                     │
                           │                          GND                     │

    STAGE 3: RECTIFIER & COMPARATOR
    ────────────────────────────────
                                 +3.3V
                                   │
                              ┌────┴────┐
                              │  100nF  │  C63 (TLV3201 decoupling)
                              └────┬────┘
                                   │
                              VCC──┤
                                   │ U7: TLV3201
    AC_SENSE ────[10kΩ R95]───┬────┤+  (Comparator) ├───────────► GPIO4
                              │    │                 │
                         ┌────┴────┐    VREF ────────┤-
                         │  100nF  │    (1.65V)      │
                         │   C65   │                 │
                         │(filter) │    └────┬───────┘
                         └────┬────┘         │
                              │             ─┴─
                             ─┴─            GND
                             GND

    VREF Divider:   +3.3V ──[100kΩ R96]──┬──[100kΩ R97]── GND
                                          │
                                          └──► VREF (~1.65V)

    Hysteresis:     GPIO4 ────[1MΩ R98]────► + input (TLV3201)

    Logic States:
    ─────────────
    Water BELOW probe → GPIO4 reads HIGH → Low Water / UNSAFE for heater
    Water AT/ABOVE probe → GPIO4 reads LOW → Level OK

    Component Values:
    ─────────────────
    U6:  OPA342UA (SOIC-8) or OPA2342UA (use one section)
         Alt: OPA207 for lower noise
    U7:  TLV3201AIDBVR (SOT-23-5)
    R91: 10kΩ 1%, 0805 (oscillator feedback)
    R92: 10kΩ 1%, 0805 (Wien bridge)
    R93: 10kΩ 1%, 0805 (Wien bridge)
    R94: 100Ω 5%, 0805 (probe current limit)
    R95: 10kΩ 5%, 0805 (AC bias)
    R96: 100kΩ 1%, 0805 (reference divider)
    R97: 100kΩ 1%, 0805 (reference divider)
    R98: 1MΩ 5%, 0805 (hysteresis)
    C60: 100nF 25V, 0805 (OPA342 decoupling)
    C61: 100nF 25V, 0805 (Wien bridge timing)
    C62: 100nF 25V, 0805 (Wien bridge timing)
    C63: 100nF 25V, 0805 (TLV3201 decoupling)
    C64: 1µF 25V, 0805 (AC coupling to probe)
    C65: 100nF 25V, 0805 (sense filter)

    ⚠️ PCB LAYOUT: GUARD RING REQUIRED
    ───────────────────────────────────
    The trace from J26 Pin 5 (Level Probe) to OPA342 input is HIGH-IMPEDANCE
    and will pick up 50/60Hz mains hum if not properly shielded.

    REQUIRED: Surround probe trace with GND guard ring on both sides.
    Place OPA342 as CLOSE as possible to J26 terminal (< 2cm trace).
    Avoid routing near relay coils or mains traces.

    Benefits of OPA342 + TLV3201:
    ─────────────────────────────
    • Uses commonly available, modern components
    • AC excitation prevents electrolysis and probe corrosion
    • Adjustable threshold via R96/R97 ratio
    • Clean hysteresis via R98
    • Low power consumption (<1mA)

    Other Switches (unchanged):
    ───────────────────────────
    R16-18: 10kΩ 5%, 0805 (or use Pico internal pull-ups)
    D12-14: PESD5V0S1BL, SOD-323
```

---

# Sheet 6: Communication Interfaces

## 6.1 ESP32 Display Module Interface

```
                    ESP32 DISPLAY MODULE INTERFACE
    ════════════════════════════════════════════════════════════════════════════

    J15: JST-XH 8-pin connector (B8B-XH-A) - Includes brew-by-weight support

                                            +5V
                                             │
                                        ┌────┴────┐
                                        │  100µF  │  Bulk cap
                                        │   C13   │  for ESP32
                                        └────┬────┘
                                             │
    J15 Pin 1 (5V) ──────────────────────────┴──────────────────────────────────

    J15 Pin 2 (GND) ────────────────────────────────────────────────────── GND


    UART TX (GPIO0 → ESP32 RX):
    ───────────────────────────
                                    ┌────┐
    GPIO0 ──────────────────────────┤ 33Ω├─────────────────── J15 Pin 3 (TX)
    (UART0_TX)                      │R40 │
                                    └────┘

    UART RX (ESP32 TX → GPIO1):
    ───────────────────────────
                                    ┌────┐
    GPIO1 ◄─────────────────────────┤ 33Ω├─────────────────── J15 Pin 4 (RX)
    (UART0_RX)                      │R41 │
                                    └────┘


    ════════════════════════════════════════════════════════════════════════
    ESP32 → PICO CONTROL (OTA Firmware Updates)
    ════════════════════════════════════════════════════════════════════════

    ESP32 updates ITSELF via WiFi OTA (standard ESP-IDF).
    ESP32 updates PICO via UART + RUN/BOOTSEL control below.
    Pico has no WiFi - relies on ESP32 as firmware update gateway.


    PICO RUN Control (J15 Pin 5 → Pico RUN):
    ─────────────────────────────────────────

                                 +3.3V
                                    │
                               ┌────┴────┐
                               │  10kΩ   │  R71
                               │ Pull-up │  (Pico has internal, add external)
                               └────┬────┘
                                    │
    Pico RUN pin ◄──────────────────┴────────────────── J15 Pin 5 (RUN)
                                                              │
                                                              │
                                                         ESP32 GPIO
                                                      (Open-Drain Output)

    • ESP32 pulls LOW → Pico resets
    • ESP32 releases (high-Z) → Pico runs normally


    PICO BOOTSEL Control (J15 Pin 6 → Pico BOOTSEL):
    ─────────────────────────────────────────────────

                                 +3.3V
                                    │
                               ┌────┴────┐
                               │  10kΩ   │  R72
                               │ Pull-up │  (BOOTSEL normally HIGH)
                               └────┬────┘
                                    │
    Pico BOOTSEL ◄──────────────────┴────────────────── J15 Pin 6 (BOOT)
                                                              │
                                                              │
                                                         ESP32 GPIO
                                                      (Open-Drain Output)

    • ESP32 holds LOW during reset → Pico enters USB bootloader
    • ESP32 releases → Pico boots normally


    WEIGHT_STOP Signal (J15 Pin 7 → GPIO21) - BREW BY WEIGHT:
    ──────────────────────────────────────────────────────────

                                 +3.3V
                                    │
                               ┌────┴────┐
                               │  10kΩ   │  R73
                               │Pull-down│  (Normally LOW - no brew stop)
                               └────┬────┘
                                    │
    GPIO21 ◄────────────────────────┴────────────────── J15 Pin 7 (WEIGHT_STOP)
    (Input)                                                   │
                                                              │
                                                         ESP32 GPIO
                                                       (Push-Pull Output)

    • ESP32 connects to Bluetooth scale (Acaia, Decent, etc.)
    • ESP32 monitors weight during brew
    • When target weight reached → ESP32 sets Pin 7 HIGH
    • Pico detects HIGH on GPIO21 → Stops pump (K2) immediately
    • ESP32 sets Pin 7 LOW → Ready for next brew


    SPARE Signal (J15 Pin 8 → GPIO22):
    ───────────────────────────────────

    GPIO22 ◄────────────────────────────────────────── J15 Pin 8 (SPARE)
    (I/O)                                                    │
                                                             │
                                                        ESP32 GPIO

    • Reserved for future expansion
    • No pull-up/down installed (configure in firmware as needed)
    • Suggested uses: Flow sensor, additional sensor, bidirectional signal


    OTA Update Sequence:
    ─────────────────────
    1. ESP32 downloads Pico firmware via WiFi
    2. ESP32 sends "ENTER_BOOTLOADER" command via UART
    3. Pico custom bootloader acknowledges
    4. ESP32 streams firmware via UART (115200 baud)
    5. Pico writes to flash
    6. ESP32 pulses RUN LOW to reset Pico
    7. Pico boots with new firmware

    Recovery (if Pico firmware corrupted):
    ───────────────────────────────────────
    1. ESP32 holds BOOTSEL LOW
    2. ESP32 pulses RUN LOW then releases
    3. Pico enters hardware bootloader
    4. ESP32 releases BOOTSEL
    5. ESP32 can reflash via custom UART protocol


    J15 Pinout Summary (8-pin):
    ───────────────────────────
    Pin 1: 5V      → ESP32 VIN (power)
    Pin 2: GND     → Common ground
    Pin 3: TX      → ESP32 RX (from Pico GPIO0)
    Pin 4: RX      ← ESP32 TX (to Pico GPIO1)
    Pin 5: RUN     ← ESP32 GPIO (to Pico RUN pin)
    Pin 6: BOOT    ← ESP32 GPIO (to Pico BOOTSEL)
    Pin 7: WGHT    ← ESP32 GPIO (to Pico GPIO21) - Brew-by-weight stop signal
    Pin 8: SPARE   ↔ ESP32 GPIO (to Pico GPIO22) - Reserved for future use

    Component Values:
    ─────────────────
    R40-41: 33Ω 5%, 0805 (UART series protection)
    R71-72: 10kΩ 5%, 0805 (RUN/BOOTSEL pull-ups)
    R73:    10kΩ 5%, 0805 (WEIGHT_STOP pull-down)
    C13:    100µF 10V, Electrolytic (ESP32 power decoupling)
```

## 6.2 Service/Debug Port

```
                        SERVICE/DEBUG PORT
    ════════════════════════════════════════════════════════════════════════════

    J16: 4-pin 2.54mm header

                            +3.3V
                              │
    J16 Pin 1 (3V3) ──────────┴───────────────────────────────────────────────

    J16 Pin 2 (GND) ──────────────────────────────────────────────────── GND

    SERVICE PORT (J16) - Shared with ESP32 on GPIO0/1:
    ─────────────────────────────────────────────────
    ⚠️ DISCONNECT ESP32 (J15) BEFORE USING SERVICE PORT

                                    ┌────┐
    GPIO0 ──────────────────────────┤ 33Ω├──┬──────────────── J15 Pin 3 (ESP32 RX)
    (UART0_TX)                      │R40 │  │
                                    └────┘  └──────────────── J16 Pin 3 (Service TX)

                                    ┌────┐
    GPIO1 ◄─────────────────────────┤ 33Ω├──┬──────────────── J15 Pin 4 (ESP32 TX)
    (UART0_RX)                      │R41 │  │
                                    └────┘  └──────────────── J16 Pin 4 (Service RX)

    I2C ACCESSORY PORT (J23) - GPIO8/9:
    ────────────────────────────────────
                  3.3V
                   │
            ┌──────┴──────┐
         [4.7kΩ]       [4.7kΩ]
          R50           R51
            │             │
    GPIO8 ──┴─────────────┼─────────────── J23 Pin 3 (SDA)
    (I2C0_SDA)            │
    GPIO9 ────────────────┴─────────────── J23 Pin 4 (SCL)
    (I2C0_SCL)

    Configuration:
    ──────────────
    Default baud rate: 115200, 8N1

    Silkscreen: "3V3 GND TX RX" or "DEBUG"
```

---

# Sheet 7: User Interface

## 7.1 Status LED

```
                            STATUS LED
    ════════════════════════════════════════════════════════════════════════════

                                    ┌────────┐
    GPIO15 ─────────────────────────┤  330Ω  ├──────────────┬────────────────
                                    │  R50   │              │
                                    └────────┘         ┌────┴────┐
                                                       │   LED   │
                                                       │  Green  │
                                                       │  LED7   │
                                                       │  0805   │
                                                       └────┬────┘
                                                            │
                                                           ─┴─
                                                           GND

    Current: (3.3V - 2.0V) / 330Ω ≈ 4mA (clearly visible)

    NOTE: Green LED chosen over blue because:
    • Blue LEDs have Vf=3.0-3.4V, leaving only 0-0.3V margin with 3.3V supply
    • Green LEDs have Vf=1.8-2.2V, providing reliable ~4mA current
    • Green matches relay indicator LEDs for consistency

    Component Values:
    ─────────────────
    R50:  330Ω 5%, 0805
    LED7: Green 0805, Vf~2.0V
```

## 7.2 Buzzer

```
                            BUZZER (Passive)
    ════════════════════════════════════════════════════════════════════════════

    Using passive buzzer with PWM for variable tones:

                                    ┌────────────┐
    GPIO19 ─────────────────────────┤   100Ω     ├──────────────┐
    (PWM)                           │    R51     │              │
                                    └────────────┘              │
                                                           ┌────┴────┐
                                                           │ Passive │
                                                           │ Buzzer  │
                                                           │   BZ1   │
                                                           └────┬────┘
                                                                │
                                                               ─┴─
                                                               GND

    Component Values:
    ─────────────────
    R51: 100Ω 5%, 0805
    BZ1: CUI CEM-1203(42), 12mm, Passive Piezo

    PWM Frequencies:
    ────────────────
    2000-2500 Hz: Ready/confirmation beeps
    1000-1500 Hz: Warnings
    500 Hz:       Error
```

---

# Sheet 8: Power Metering (Universal External Interface)

## 8.1 Universal External Power Meter Interface

```
                    UNIVERSAL EXTERNAL POWER METER INTERFACE
    ════════════════════════════════════════════════════════════════════════════

    ✅ NO HIGH VOLTAGE THROUGH CONTROL PCB FOR METERING!
    ✅ External modules handle their own mains connections
    ✅ Supports TTL UART (direct) or RS485 (differential) meters
    ✅ Compatible with PZEM-004T, JSY-MK-163T/194T, Eastron SDM, and more

    ═══════════════════════════════════════════════════════════════════════════
    OFF-BOARD POWER METER DESIGN (User mounts module externally)
    ═══════════════════════════════════════════════════════════════════════════

    SYSTEM TOPOLOGY:
    ────────────────

    ┌────────────────────────────────────────────────────────────────────────┐
    │                         CONTROL PCB                                    │
    │                                                                        │
    │   J17 (JST-XH 6-pin)                                                   │
    │   ┌─────┬─────┬─────┬─────┬─────┬─────┐                               │
    │   │ 3V3 │ 5V  │ GND │ RX  │ TX  │DE/RE│                               │
    │   │  1  │  2  │  3  │  4  │  5  │  6  │                               │
    │   └──┬──┴──┬──┴──┬──┴──┬──┴──┬──┴──┬──┘                               │
    │      │     │     │     │     │     │                                   │
    │      │     │     │     │     │     └──► GPIO20 (RS485 DE/RE control)  │
    │      │     │     │     │     └────────► GPIO6 (UART TX to meter)       │
    │      │     │     │     └──────────────► GPIO7 (UART RX from meter)     │
    │      │     │     └────────────────────► GND                            │
    │      │     └──────────────────────────► 5V (for 5V meters)             │
    │      └────────────────────────────────► 3.3V (for 3.3V meters)         │
    │                                                                        │
    │   ═══════════════════════════════════════════════════════════════════  │
    │                                                                        │
    │   RS485 TRANSCEIVER (U8: MAX3485) - For Industrial Meters              │
    │   ───────────────────────────────────────────────────────              │
    │                                                                        │
    │                               +3.3V                                    │
    │                                 │                                      │
    │                            ┌────┴────┐                                 │
    │                            │  100nF  │ C70                             │
    │                            └────┬────┘                                 │
    │                            VCC──┤                                      │
    │                                 │  U8: MAX3485                         │
    │                       ┌─────────┴─────────┐                            │
    │   GPIO6 (TX) ────────►│ DI            A   │◄───► J17-4 (RS485 mode)    │
    │                       │                   │      or METER_RX (TTL)     │
    │   GPIO7 (RX) ◄────────│ RO            B   │◄───► J17-5 (RS485 mode)    │
    │                       │                   │      or METER_TX (TTL)     │
    │   GPIO20 (DE/RE) ────►│ DE/RE         GND │                            │
    │                       │                   │                            │
    │                       └─────────┬─────────┘                            │
    │                                ─┴─                                     │
    │                                GND                                     │
    │                                                                        │
    │   TERMINATION (Jumper Selectable):                                     │
    │   ─────────────────────────────────                                    │
    │   J17-4 (A) ────┬────[120Ω R99]────┬──── J17-5 (B)                     │
    │                 └────── JP1 ───────┘                                   │
    │                    (Solder jumper)                                     │
    │                                                                        │
    │   JP1 CLOSED: 120Ω termination enabled (for long RS485 runs)           │
    │   JP1 OPEN:   No termination (for short cables or TTL mode)            │
    │                                                                        │
    └────────────────────────────────────────────────────────────────────────┘


    EXTERNAL POWER METER (User-mounted, off-board):
    ────────────────────────────────────────────────

    ┌────────────────────────────────────────────────────────────────────────┐
    │              EXTERNAL POWER METER MODULE                               │
    │              (PZEM-004T, JSY-MK-163T, Eastron SDM, etc.)              │
    │                                                                        │
    │   ┌──────────────────┐     ┌──────────────────────────────────────┐   │
    │   │   LV INTERFACE   │     │         HV INTERFACE                 │   │
    │   │   (JST or wire)  │     │    (Screw terminals on module)       │   │
    │   │                  │     │                                      │   │
    │   │   5V ◄───────────│     │   L ◄──────── Machine Mains Live     │   │
    │   │   GND ◄──────────│     │   N ◄──────── Machine Mains Neutral  │   │
    │   │   TX ────────────│     │                                      │   │
    │   │   RX ◄───────────│     │   CT+ ◄────┬───── CT Clamp           │   │
    │   │                  │     │   CT- ◄────┘      (on Live wire)     │   │
    │   └──────────────────┘     └──────────────────────────────────────┘   │
    │         │                              │                              │
    │         │ JST cable                    │ User wires directly          │
    │         │ to J17                       │ to mains                     │
    │         ▼                              ▼                              │
    │   CONTROL PCB                    MACHINE WIRING                       │
    │                                  (NOT through PCB)                    │
    │                                                                        │
    └────────────────────────────────────────────────────────────────────────┘


    ⚠️  CRITICAL: NO HIGH VOLTAGE ON CONTROL PCB FOR METERING
    ────────────────────────────────────────────────────────────
    • External meter module handles its own L, N, CT connections
    • Control PCB provides ONLY 5V/3.3V power and UART/RS485 data
    • User wires machine mains directly to external module terminals
    • CT clamp connects directly to external module (not through PCB)


    SUPPORTED MODULES:
    ──────────────────
    │ Module          │ Baud  │ Protocol   │ Interface │ CT Type        │
    │─────────────────│───────│────────────│───────────│────────────────│
    │ PZEM-004T V3    │ 9600  │ Modbus RTU │ TTL UART  │ Split-core 100A│
    │ JSY-MK-163T     │ 4800  │ Modbus RTU │ TTL UART  │ Internal shunt │
    │ JSY-MK-194T     │ 4800  │ Modbus RTU │ TTL UART  │ Dual-channel   │
    │ Eastron SDM120  │ 2400  │ Modbus RTU │ RS485     │ DIN-rail CT    │
    │ Eastron SDM230  │ 9600  │ Modbus RTU │ RS485     │ DIN-rail CT    │


    J17 PINOUT (JST-XH 6-pin):
    ──────────────────────────
    Pin 1: 3V3    → Power for 3.3V logic modules
    Pin 2: 5V     → Power for 5V modules (PZEM, JSY)
    Pin 3: GND    → System ground
    Pin 4: RX     → GPIO7 (from meter TX, via level shifter)
    Pin 5: TX     → GPIO6 (to meter RX, via 33Ω R44)
    Pin 6: DE/RE  → GPIO20 (RS485 direction control)


    ⚠️ v2.22 CRITICAL FIX: 5V→3.3V LEVEL SHIFTING (PZEM RX LINE)
    ─────────────────────────────────────────────────────────────
    PZEM-004T outputs 5V TTL. RP2350 GPIO is NOT 5V tolerant!
    Without level shifting, 5V signals cause GPIO damage over time.

    SOLUTION: Resistive voltage divider on J17-4 (RX input):

         J17 Pin 4 (5V from PZEM TX)
              │
         ┌────┴────┐
         │  2.2kΩ  │  R45 (upper)
         │   1%    │
         └────┬────┘
              │
              ├──────[33Ω R45B]─────────────► GPIO7 (PZEM_RX)
              │
         ┌────┴────┐
         │  3.3kΩ  │  R45A (lower)
         │   1%    │
         └────┬────┘
              │
             GND

    V_out = 5V × 3.3k / (2.2k + 3.3k) = 5V × 0.6 = 3.0V ✓


    COMPONENT VALUES:
    ─────────────────
    U8:     MAX3485ESA+ or SP3485EN-L/TR (SOIC-8 or SOT-23-8)
    C70:    100nF 25V Ceramic, 0805 (U8 decoupling)
    R44:    33Ω 5%, 0805 (TX series protection)
    R45:    2.2kΩ 1%, 0805 (⚠️ v2.22: RX level shift upper)
    R45A:   3.3kΩ 1%, 0805 (⚠️ v2.22: RX level shift lower)
    R45B:   33Ω 5%, 0805 (RX series after divider)
    R99:    120Ω 1%, 0805 (RS485 termination, via JP1)
    JP1:    Solder jumper for termination (default: open)
    J17:    JST-XH 6-pin header (B6B-XH-A)


    TTL UART MODE (PZEM, JSY - Most Common):
    ────────────────────────────────────────
    • U8 can be bypassed or GPIO6/7 routed directly to J17-4/5
    • GPIO20 (DE/RE) not connected or held LOW
    • RX line level-shifted via on-board resistor divider (safe for RP2350)


    RS485 MODE (Eastron, Industrial):
    ──────────────────────────────────
    • GPIO6/7 connect through U8 (MAX3485)
    • GPIO20 controls DE/RE for half-duplex direction
    • J17-4/5 become differential A/B pair
    • Enable JP1 for 120Ω termination on long cable runs


    FIRMWARE AUTO-DETECTION:
    ────────────────────────
    On startup, firmware scans:
    1. 9600 baud → Try PZEM registers
    2. 4800 baud → Try JSY registers
    3. 2400 baud → Try Eastron registers
    4. 19200 baud → Try other Modbus devices

    Successfully detected meter configuration saved to flash.

```

---

# Sheet 9: Protection & Filtering

## 9.1 5V Rail Protection

```
                        5V RAIL TRANSIENT PROTECTION
    ════════════════════════════════════════════════════════════════════════════

    From AC/DC                                              To 5V Distribution
    Module                                                  (Pico, Relays, etc.)
        │                                                           │
        │      ┌────────────┐                                       │
        └──────┤  Ferrite   ├───────────┬───────────────────────────┘
               │   Bead     │           │
               │   FB2      │           │
               └────────────┘           │
                                   ┌────┴────┐    ┌────┴────┐
                                   │  D20    │    │  C2     │
                                   │  TVS    │    │  100µF  │
                                   │ SMBJ5.0A│    │  16V    │
                                   └────┬────┘    └────┬────┘
                                        │              │
                                       ─┴─            ─┴─
                                       GND            GND

    Component Values:
    ─────────────────
    FB2: Ferrite Bead, 600Ω @ 100MHz, 0805 (optional, for noise)
    D20: SMBJ5.0A, SMB package, 5V TVS diode (absorbs transients)
    C2:  470µF 6.3V Polymer (low ESR, better for hot environment inside machine)
```

---

# Net List Summary

```
                            NET LIST SUMMARY
    ════════════════════════════════════════════════════════════════════════════

    POWER NETS:
    ───────────
    +5V          → Pico VSYS, Relay coils, SSR drivers, ESP32 (J15-1), LED anodes
    +3.3V        → Pico 3V3, Digital I/O, pull-ups, MAX3485 (U8)
    +3.3V_ANALOG → ADC reference, NTC dividers, MAX31855
    GND          → System ground (isolated from mains PE)

    HIGH VOLTAGE NETS (⚠️ MAINS):
    ─────────────────────────────
    L_IN         → J1-L (Mains Live Input)
    L_FUSED      → After F1 fuse (10A) - to relay COMs only
    N            → J1-N (Mains Neutral)
    PE           → J1-PE (Protective Earth, to chassis)

    NOTE: NO heater current or power metering HV flows through control PCB.
    Heaters connect to external SSRs, power meters connect to mains
    via their own terminals. PCB provides only LV control signals.

    RELAY OUTPUT NETS:
    ──────────────────
    K1_COM       → Relay K1 Common (internal to L_FUSED)
    K2_COM       → Relay K2 Common (internal to L_FUSED)
    K3_COM       → Relay K3 Common (internal to L_FUSED)

    GPIO NETS:
    ──────────
    GPIO0  → ESP32_TX (J15-3)
    GPIO1  → ESP32_RX (J15-4)
    GPIO2  → WATER_SW (J26-1, S1)
    GPIO3  → TANK_LVL (J26-3, S2)
    GPIO4  → STEAM_LVL (J26-5, S3 via comparator)
    GPIO5  → BREW_SW (J26-6, S4)
    GPIO6  → METER_TX (UART to meter RX / RS485 DI, J17-5)
    GPIO7  → METER_RX (UART from meter TX / RS485 RO, J17-4)
    GPIO8  → I2C0_SDA (J23-3, with 4.7kΩ pull-up)
    GPIO9  → I2C0_SCL (J23-4, with 4.7kΩ pull-up)
    GPIO10 → K1_DRIVE (relay coil, output to J2 spade - 220V)
    GPIO11 → K2_DRIVE (relay coil, output to J3 spade - 220V)
    GPIO12 → K3_DRIVE (relay coil, output to J4 spade - 220V)
    GPIO13 → SSR1_DRIVE
    GPIO14 → SSR2_DRIVE
    GPIO15 → STATUS_LED
    GPIO16 → SPI_MISO (MAX31855 DO)
    GPIO17 → SPI_CS (MAX31855 CS)
    GPIO18 → SPI_SCK (MAX31855 CLK)
    GPIO19 → BUZZER
    GPIO20 → RS485_DE_RE (MAX3485 direction control, J17-6)
    GPIO21 → WEIGHT_STOP (J15-7, ESP32 brew-by-weight signal)
    GPIO22 → SPARE/EXPANSION (J15-8, available for future use)
    GPIO23-25 → INTERNAL (Pico 2 module - NOT on header)
    GPIO26 → ADC0_BREW_NTC (J26-8/9)
    GPIO27 → ADC1_STEAM_NTC (J26-10/11)
    GPIO28 → ADC2_PRESSURE (from J26-16 voltage divider)
    RUN    → SW1_RESET
    BOOTSEL→ SW2_BOOT

    220V AC RELAY OUTPUTS (6.3mm Spade Terminals):
    ──────────────────────────────────────────────
    J2-NO  → Relay K1 N.O. output (Water LED, 220V ≤100mA)
    J3-NO  → Relay K2 N.O. output (Pump, 220V 5A)
    J4-NO  → Relay K3 N.O. output (Solenoid, 220V ~0.5A)

    J17 POWER METER LV INTERFACE (JST-XH 6-pin):
    ─────────────────────────────────────────────
    J17-1  (3V3)   → 3.3V power for logic-level meters
    J17-2  (5V)    → 5V power for PZEM, JSY modules
    J17-3  (GND)   → System ground
    J17-4  (RX)    → GPIO7 (from meter TX, via 33Ω)
    J17-5  (TX)    → GPIO6 (to meter RX, via 33Ω)
    J17-6  (DE/RE) → GPIO20 (RS485 direction control)

    J24 POWER METER HV TERMINALS (Screw Terminal 3-pos, 5.08mm):
    ────────────────────────────────────────────────────────────
    J24-1  (L)     → Live FUSED (from L_FUSED bus, after F1)
    J24-2  (N)     → Neutral (from N bus)
    J24-3  (PE)    → Protective Earth (optional, for DIN rail meters)

    NOTE: GPIO23-25 and GPIO29 are used internally by Pico 2 module
          and are NOT available on the 40-pin header.
          For expansion, use GPIO22 via J15 Pin 8 (SPARE).

    J26 UNIFIED LOW-VOLTAGE TERMINAL BLOCK (22-pos):
    ─────────────────────────────────────────────────
    J26-1  (S1)    → Water Reservoir Switch Signal → GPIO2
    J26-2  (S1-G)  → Water Reservoir Switch GND
    J26-3  (S2)    → Tank Level Sensor Signal → GPIO3
    J26-4  (S2-G)  → Tank Level Sensor GND
    J26-5  (S3)    → Steam Boiler Level Probe → GPIO4 (via comparator)
    J26-6  (S4)    → Brew Handle Switch Signal → GPIO5
    J26-7  (S4-G)  → Brew Handle Switch GND
    J26-8  (T1)    → Brew NTC Signal → ADC0 via divider
    J26-9  (T1-G)  → Brew NTC GND
    J26-10 (T2)    → Steam NTC Signal → ADC1 via divider
    J26-11 (T2-G)  → Steam NTC GND
    J26-12 (TC+)   → Thermocouple + → MAX31855
    J26-13 (TC-)   → Thermocouple - → MAX31855
    J26-14 (P-5V)  → Pressure transducer 5V
    J26-15 (P-GND) → Pressure transducer GND
    J26-16 (P-SIG) → Pressure signal → ADC2 via divider
    J26-17 (SSR1+) → +5V (Brew heater SSR power)
    J26-18 (SSR1-) → SSR1_NEG (Brew heater SSR trigger)
    J26-19 (SSR2+) → +5V (Steam heater SSR power)
    J26-20 (SSR2-) → SSR2_NEG (Steam heater SSR trigger)
    J26-21 (GND)   → Spare ground
    J26-22 (GND)   → Spare ground

    ⚠️ CT CLAMP: Connects directly to external power meter module (not on J26)

    SOLDER JUMPERS:
    ────────────────
    JP1 → RS485 120Ω termination (default: OPEN)
    JP2 → Brew NTC selection (OPEN=50kΩ ECM, CLOSE=10kΩ Rocket/Gaggia)
    JP3 → Steam NTC selection (OPEN=50kΩ ECM, CLOSE=10kΩ Rocket/Gaggia)
```

---

_End of Schematic Reference Document_
