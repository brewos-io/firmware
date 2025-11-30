# ECM Synchronika Control Board - Schematic Reference
## Detailed Circuit Schematics for PCB Design

**Board Size:** 130mm × 80mm (2-layer, 2oz copper)  
**Enclosure:** 150mm × 100mm mounting area  
**Revision:** Matches ECM_Control_Board_Specification_v2.md (v2.16)

---

## ⚠️ KEY DESIGN CHANGES IN v2.16

1. **OPA342 + TLV3201** for steam boiler level probe (AC sensing with common components)
2. **PZEM-004T v3.0** external power meter (NO high current through PCB)
3. **HLK-5M05** power supply (3A, compact 16mm height)
4. **10A fuse** with PCB mount holder (Littelfuse 01000056Z)
5. **Optimized NTC pull-ups**: R1=470Ω (brew 90°C), R2=150Ω (steam 135°C)
6. **Pressure divider R4=4.7kΩ** (91% ADC range utilization)
7. **Snubbers MANDATORY** for K2 (Pump) and K3 (Solenoid)
8. **Mounting holes**: MH1=PE star point (PTH), MH2-4=NPTH (isolated)
9. **K4 trace width 1.5mm** (future-proofed for high-current loads)

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

                                    U2: Hi-Link HLK-5M05
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
    • HLK-5M05 is 50×27×16mm - verify clearance to fuse holder

    Component Values:
    ─────────────────
    U2:  Hi-Link HLK-5M05 (5V 3A) - adequate for ~1.1A peak load
         Alt: Mean Well IRM-20-5 (5V 4A) if more headroom needed
    C2:  100µF 16V Electrolytic, Low ESR, 6.3mm diameter
```

## 1.3 5V to 3.3V LDO Regulator

```
                                3.3V LDO REGULATOR
    ════════════════════════════════════════════════════════════════════════════

                            U3: AP2112K-3.3TRG1
                           ┌──────────────────┐
                           │                  │
    +5V ─────┬─────────────┤ VIN        VOUT  ├───────┬───────────────────► +3.3V
             │             │                  │       │
         C3  │             │       GND        │   C4  │   C5
       ┌─────┴─────┐       │        │         │ ┌─────┴─────┐ ┌─────┐
       │   100µF   │       └────────┼─────────┘ │   47µF    │ │100nF│
       │   16V     │                │           │   10V     │ │ 25V │
       │   Alum.   │                │           │  Ceramic  │ │     │
       └─────┬─────┘                │           │   1206    │ └──┬──┘
             │                      │           └─────┬─────┘    │
            ─┴─                    ─┴─               ─┴─        ─┴─
            GND                    GND               GND        GND

    
                        Ferrite Bead for Analog Section
                        ───────────────────────────────
                        
    +3.3V ──────────┬───[FB1: 600Ω @ 100MHz]───┬──────────────────► +3.3V_ANALOG
                    │                          │
                C6  │                      C7  │
              ┌─────┴─────┐              ┌─────┴─────┐
              │   100nF   │              │   22µF    │
              │    25V    │              │   10V     │
              │  Ceramic  │              │  Ceramic  │
              └─────┬─────┘              └─────┬─────┘
                   ─┴─                        ─┴─
                   GND                        GND

    Component Values:
    ─────────────────
    U3:  Diodes Inc. AP2112K-3.3TRG1, 600mA, SOT-23-5
    C3:  100µF 16V Electrolytic
    C4:  47µF 10V Ceramic, 1206 (X5R or X7R)
    C5:  100nF 25V Ceramic, 0805
    FB1: Murata BLM18PG601SN1D, 600Ω @ 100MHz, 0603
    C6:  100nF 25V Ceramic, 0805
    C7:  22µF 10V Ceramic, 1206
```

---

# Sheet 2: Microcontroller (Raspberry Pi Pico)

## 2.1 Pico Module & Decoupling

```
                            RASPBERRY PI PICO MODULE
    ════════════════════════════════════════════════════════════════════════════

                                U1: Raspberry Pi Pico
    
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
               │     │          │ GP6  GP20 ├────────────────┼─► K4_RELAY
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
    
    (Identical circuit for K1, K2, K3. K2 uses 16A relay for pump.)

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

    Relay Contact Connections (to 6.3mm Spade Terminals):
    ─────────────────────────────────────────────────────

    K1 (Water LED):           K2 (Pump):                K3 (Solenoid):
    J2-COM ── K1-COM         J3-COM ── K2-COM          J4-COM ── K3-COM
    J2-NO  ── K1-NO          J3-NO  ── K2-NO           J4-NO  ── K3-NO

    Component Values:
    ─────────────────
    K1, K3: HF46F-G/005-HS1 (5V coil, 10A contacts, SPST-NO)
    K2:     Omron G5LE-1A4 DC5 (5V coil, 16A contacts, SPST-NO) - FOR PUMP
    D1-D3:  1N4007 (1A, 1000V) or LL4007 MINIMELF
    Q1-Q3:  MMBT2222A (SOT-23)
    LED1-3: Green 0805, Vf~2.0V
    R20-22: 1kΩ 5% 0805 (transistor base)
    R30-32: 470Ω 5% 0805 (LED current limit - gives 6.4mA)
    R10-12: 10kΩ 5% 0805 (pull-down, ensures relay off at boot)

    ═══════════════════════════════════════════════════════════════════════════
    RC SNUBBER CIRCUITS (Inductive Load Protection)
    ═══════════════════════════════════════════════════════════════════════════

    ⚠️ MANDATORY for K2 (Pump) and K3 (Solenoid) - prevents EMI crashes!

    Across relay contacts (HV side):
    ─────────────────────────────────

         Relay NO ──────┬───────────────────────────► To Load
                        │
                   ┌────┴────┐      ┌────────┐
                   │  100nF  │──────┤  100Ω  │
                   │   X2    │      │   2W   │
                   │  275VAC │      │        │
                   └────┬────┘      └────┬───┘
                        │               │
                        └───────┬───────┘
                                │
         Relay COM ─────────────┴───────────────────── From L_FUSED

    Snubber Components:
    ────────────────────
    K2 (Pump):     C50 (100nF X2) + R80 (100Ω 2W) - MANDATORY
    K3 (Solenoid): C51 (100nF X2) + R81 (100Ω 2W) - MANDATORY
    K1 (LED):      C52 (100nF X2) + R82 (100Ω 2W) - DNP (footprint only)
    K4 (Spare):    C53 (100nF X2) + R83 (100Ω 2W) - DNP (footprint only)

    Snubber Component Values:
    ─────────────────────────
    C50-C53: 100nF X2, 275V AC (Vishay MKP1840 or TDK B32922)
    R80-R83: 100Ω 2W, Metal Film (1210 package or through-hole)
```

## 3.2 Spare Relay K4 (SPDT) - Universal, Floating Contacts

```
                            SPARE RELAY K4 (SPDT)
                    ⚠️ CONTACTS FLOATING - NOT WIRED TO MAINS
    ════════════════════════════════════════════════════════════════════════════

    (Same driver circuit as K1-K3, but relay contacts are NOT pre-wired)
    (User can wire for ANY voltage ≤250V AC/DC, 10A max)

                                        +5V
                                         │
               ┌─────────────────────────┴─────────────────────────┐
               │                                                   │
          ┌────┴────┐                                         ┌────┴────┐
          │  Relay  │                                         │  470Ω   │
          │  Coil   │                                         │   R33   │
          │   5V    │                                         │  (LED)  │
          │   K4    │                                         └────┬────┘
          │  SPDT   │                                              │
          └────┬────┘                                         ┌────┴────┐
               │                                              │   LED   │
          ┌────┴────┐                                         │  Green  │
          │   D4    │ ◄── Flyback diode                       │  LED4   │
          │ 1N4007  │                                         └────┬────┘
          └────┬────┘                                              │
               │                                                   │
               └───────────────────────┬───────────────────────────┤
                                       │                           │
                                  ┌────┴────┐                      │
                                  │    C    │                      │
                                  │   Q4    │  MMBT2222A           │
                                  │   NPN   │                      │
    GPIO20 ─────────────[1kΩ]────►│    B    │                      │
                         R23      │    E    ├──────────────────────┘
                           │      └────┬────┘
                      ┌────┴────┐      │
                      │  10kΩ   │     ─┴─
                      │   R13   │     GND
                      └────┬────┘
                          ─┴─
                          GND

    K4 SPDT Contact Connections (FLOATING - Screw Terminals):
    ──────────────────────────────────────────────────────────
    J9: 3-position screw terminal (5.08mm pitch)
    
    J9-COM ── K4-COM (Common)      ─┐
    J9-NO  ── K4-NO  (Normally Open) ├─► ALL FLOATING - User wires as needed
    J9-NC  ── K4-NC  (Normally Closed)─┘
    
    ⚠️  K4 contacts are NOT connected to mains L_FUSED bus!
        Safe for: 5V, 12V, 24V DC, or 220V AC - user provides power.
        Maximum rating: 250V AC/DC, 10A

    Component Values:
    ─────────────────
    K4:  Omron G5LE-1 DC5 (5V coil, 10A contacts, SPDT)
    J9:  Phoenix Contact MKDS 1/3-5.08 (3-pos screw terminal)
    D4:  1N4007 (1A, 1000V)
    Q4:  MMBT2222A (SOT-23)
    LED4: Green 0805, Vf~2.0V
    R23: 1kΩ 5% 0805 (transistor base)
    R33: 470Ω 5% 0805 (LED current limit)
    R13: 10kΩ 5% 0805 (pull-down)
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
    │  J18-1 or J19-1  │                                      │ LED5/6  │
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
    │  J18-2 or J19-2  │                                           │
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
        J18-1 (SSR+) ── SSR Input (+)  ── from 5V rail
        J18-2 (SSR-) ── SSR Input (-)  ── to transistor collector
        SSR AC Load side: Connected via EXISTING MACHINE WIRING (not through PCB)

    SSR2 (Steam Heater):
        J19-1 (SSR+) ── SSR Input (+)  ── from 5V rail
        J19-2 (SSR-) ── SSR Input (-)  ── to transistor collector
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

```
                        NTC THERMISTOR INPUT CIRCUITS
    ════════════════════════════════════════════════════════════════════════════

    ⚠️ EACH SENSOR HAS DIFFERENT PULL-UP - OPTIMIZED FOR TARGET TEMPERATURE
    ⚠️ CONFIGURED FOR 50kΩ NTC SENSORS (ECM Synchronika standard)

    BREW NTC (GPIO26/ADC0)                  STEAM NTC (GPIO27/ADC1)
    Target: 90-100°C                        Target: 125-150°C

         +3.3V_ANALOG                            +3.3V_ANALOG
              │                                       │
         ┌────┴────┐                             ┌────┴────┐
         │  3.3kΩ  │  R1 (Brew)                  │  1.2kΩ  │  R2 (Steam)
         │  ±1%    │  Optimized for 93°C         │  ±1%    │  Optimized for 135°C
         └────┬────┘                             └────┬────┘
              │                                       │
    J10 ──────┼────┬───────► GPIO26           J11 ───┼────┬───────► GPIO27
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
    J10/11 Pin 1 ──┬──────────────────────────────────────────► To ADC
                   │
              ┌────┴────┐
              │ D10/D11 │  PESD5V0S1BL (ESD clamp)
              └────┬────┘
                  ─┴─
                  GND

    Component Values:
    ─────────────────
    R1:      3.3kΩ ±1%, 0805 (Brew NTC, optimized for 93°C)
    R2:      1.2kΩ ±1%, 0805 (Steam NTC, optimized for 135°C)
    R5-R6:   1kΩ 1%, 0805 (series protection)
    C8-C9:   100nF 25V, 0805, Ceramic
    D10-D11: PESD5V0S1BL, SOD-323 (bidirectional ESD clamp)
    
    NTC Sensors: 50kΩ @ 25°C, B25/85 ≈ 3950K (ECM Synchronika standard)
    
    Resolution at Target Temps:
    ───────────────────────────
    • Brew (93°C):  ~31 ADC counts/°C → 0.032°C resolution
    • Steam (135°C): ~25 ADC counts/°C → 0.04°C resolution
```

## 5.2 K-Type Thermocouple Input

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
    J12 Pin 1 ───────┤ T+   (1)        VCC (8) ├───────────────────── +3.3V
    (TC+)            │                         │
                     │                         │
    J12 Pin 2 ───┬───┤ T-   (2)        SCK (7) ├───────────────────── GPIO18 (SPI_SCK)
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
    J12: 2-position screw terminal, 5.08mm pitch
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

## 5.3 Pressure Transducer Input (J13 - Amplified 0.5-4.5V)

```
                    PRESSURE TRANSDUCER INPUT (AMPLIFIED TYPE)
    ════════════════════════════════════════════════════════════════════════════

    J13: 3-position screw terminal (for 0.5-4.5V amplified transducers)
    
                            +5V
                             │
    J13 Pin 1 ───────────────┴────────────────────────────────── Transducer VCC
    (5V)                                                         (Red wire)

    J13 Pin 2 ─────────────────────────────────────────────────── Transducer GND
    (GND)           │                                            (Black wire)
                   ─┴─
                   GND

    J13 Pin 3 ──────────────────────────┬─────────────────────── Transducer Signal
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

## 5.4 Digital Switch Inputs

```
                        DIGITAL SWITCH INPUTS
    ════════════════════════════════════════════════════════════════════════════

    (Identical circuit for Water Reservoir, Tank Level, Brew Handle switches)

                            +3.3V
                              │
                         ┌────┴────┐
                         │  10kΩ   │  Pull-up
                         │  R16-18 │  (or use Pico internal)
                         └────┬────┘
                              │
    J5/J6/J8 Pin 1 ───────────┼───────────────────────┬──────────────► GPIO 2/3/5
    (Switch Signal)           │                       │
                              │                  ┌────┴────┐
                         ┌────┴────┐             │  D12-14 │  ESD
                         │ Switch  │             │  PESD   │  Clamp
                         │  N.O.   │             └────┬────┘
                         └────┬────┘                  │
                              │                       │
    J5/J6/J8 Pin 2 ───────────┴───────────────────────┴──────────────── GND
    (Switch GND)


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
    AC_OUT ───[100Ω R94]───┬────────────────────► J7 (Level Probe)           │
           (current limit) │                      6.3mm spade                 │
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
    The trace from J7 (Level Probe) to OPA342 input is HIGH-IMPEDANCE
    and will pick up 50/60Hz mains hum if not properly shielded.
    
    REQUIRED: Surround probe trace with GND guard ring on both sides.
    Place OPA342 as CLOSE as possible to J7 connector (< 2cm trace).
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

# Sheet 8: Power Metering (PZEM-004T External)

## 8.1 PZEM-004T v3.0 External Power Meter

```
                    PZEM-004T v3.0 EXTERNAL POWER METERING
    ════════════════════════════════════════════════════════════════════════════

    ✅ NO HIGH CURRENT THROUGH CONTROL PCB!
    ✅ Power metering via external CT clamp (non-invasive)
    ✅ UART interface (9600 baud Modbus-RTU)

    ═══════════════════════════════════════════════════════════════════════════
    PZEM-004T-100A-D-P DIRECT MOUNT (Plugs into PCB via two female headers)
    ═══════════════════════════════════════════════════════════════════════════

    PZEM Module (user-supplied, plugs directly into PCB):
    ─────────────────────────────────────────────────────

               J24 (HV+CT)                      J17 (LV/UART)
               Left Side                        Right Side
                  │                                │
       ┌──────────┴────────────────────────────────┴──────────┐
       │               PZEM-004T-100A-D-P                     │
       │   ┌────┐                                 ┌─────┐     │
       │   │▼▼▼▼│  ← Pin headers                 │▼▼▼▼▼│     │
       └───┴────┴─────────────────────────────────┴─────┴─────┘
            ║║║║                                    ║║║║║
   ═════════╩╩╩╩════════════════════════════════════╩╩╩╩╩═════  ← Control PCB
           J24                                       J17
        (4-pin)                                    (5-pin)

    CT CLAMP CONNECTION:
    ────────────────────
    Machine Live wire ──────────────────────────────► To all loads
                          │
                     ┌────┴────┐
                     │   CT    │  ← Clamp around Live wire (non-invasive)
                     │  Clamp  │    User-supplied with PZEM module
                     └────┬────┘
                          │ (2-wire cable)
                          └─────────────────────► J25 Screw Terminal (on PCB)
                                                       │
                                                       └──► Routed to J24 CT+/CT-

    ═══════════════════════════════════════════════════════════════════════════

    PZEM-004T Specifications:
    ─────────────────────────
    • Voltage Range: 80-260V AC
    • Current Range: 0-100A (via CT clamp)
    • Accuracy: ±0.5% (pre-calibrated)
    • Interface: UART 9600 baud, Modbus-RTU
    • Measurements: V, I, P, E (energy), PF, Frequency

    Control PCB Power Distribution (Simplified):
    ─────────────────────────────────────────────
    J1-L (Live) ──►[FUSE F1: 10A]──► L_FUSED bus
                                        │
                                        ├──► K1 COM (Water LED valve)
                                        ├──► K2 COM (Pump ~5A)
                                        └──► K3 COM (3-Way Valve)

    K4 (Spare): Contacts FLOATING via J9 screw terminals.
                NOT connected to L_FUSED. User wires as needed.

    Note: Heaters connect to external SSRs via machine's existing wiring.
          SSR control signals only come from PCB (J18, J19).
          NO heater current flows through control PCB.

    J17 Pinout (UART - Right side of PZEM):
    ───────────────────────────────────────
    Pin 1: 5V (to PZEM VCC)
    Pin 2: RX (GPIO7 ← PZEM TX) - 33Ω series resistor
    Pin 3: TX (GPIO6 → PZEM RX) - 33Ω series resistor
    Pin 4: CF (not connected - pulse output)
    Pin 5: GND (to PZEM GND)

    J24 Pinout (HV+CT - Left side of PZEM):
    ───────────────────────────────────────
    Pin 1: CT+ (routed to J25 Pin 1)
    Pin 2: CT- (routed to J25 Pin 2)
    Pin 3: N (from N bus)
    Pin 4: L (from L_FUSED bus) ⚠️ 220V!

    J25 Pinout (CT Clamp Screw Terminal):
    ─────────────────────────────────────
    Pin 1: CT+ (from J24 Pin 1)
    Pin 2: CT- (from J24 Pin 2)

    Component Values:
    ─────────────────
    J17:     Female Header 1×5 2.54mm
    J24:     Female Header 1×4 2.54mm (⚠️ HV!)
    J25:     Screw Terminal 2-pos 5.08mm
    R44-45:  33Ω 5%, 0805 (UART series protection)

    ⚠️ CRITICAL: J24 MILLING SLOT REQUIREMENT
    ─────────────────────────────────────────
    J24 carries Mains L and N on adjacent pins (2.54mm pitch).
    Mill a 1mm wide slot between Pin 3 (N) and Pin 4 (L) for safety.
    
    J24 Top View:
    ┌─────────────────────────────┐
    │ Pin1  Pin2  Pin3 ║║ Pin4   │
    │  CT+   CT-    N  ║║   L    │  ← 1mm slot between N and L
    └─────────────────────────────┘
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
    C2:  100µF 16V Electrolytic (bulk decoupling)
```

---

# Net List Summary

```
                            NET LIST SUMMARY
    ════════════════════════════════════════════════════════════════════════════

    POWER NETS:
    ───────────
    +5V          → Pico VSYS, Relay coils, SSR drivers, ESP32 (J15-1), LED anodes
    +3.3V        → Pico 3V3, Digital I/O, pull-ups
    +3.3V_ANALOG → ADC reference, NTC dividers, MAX31855
    GND          → System ground (isolated from mains PE)
    
    HIGH VOLTAGE NETS (⚠️ MAINS):
    ─────────────────────────────
    L_IN         → J1-L (Mains Live Input)
    L_FUSED      → After F1 fuse (10A) - to relay COMs only
    N            → J1-N (Mains Neutral)
    PE           → J1-PE (Protective Earth, to chassis)

    NOTE: With PZEM-004T external power metering, NO heater current
    flows through the control PCB. Heaters connect directly to
    external SSRs via machine's existing wiring.

    RELAY OUTPUT NETS:
    ──────────────────
    K1_COM       → Relay K1 Common (internal to L_FUSED)
    K2_COM       → Relay K2 Common (internal to L_FUSED)
    K3_COM       → Relay K3 Common (internal to L_FUSED)
    K4_COM       → Relay K4 Common (J9-COM, FLOATING - user wires)

    GPIO NETS:
    ──────────
    GPIO0  → ESP32_TX (J15-3)
    GPIO1  → ESP32_RX (J15-4)
    GPIO2  → WATER_SW (J5-1)
    GPIO3  → TANK_LVL (J6-1)
    GPIO4  → STEAM_LVL (J7-1)
    GPIO5  → BREW_SW (J8-1)
    GPIO6  → PZEM_TX (UART to PZEM RX, J17-3)
    GPIO7  → PZEM_RX (UART from PZEM TX, J17-4)
    GPIO8  → I2C0_SDA (J23-3, with 4.7kΩ pull-up)
    GPIO9  → I2C0_SCL (J23-4, with 4.7kΩ pull-up)
    GPIO10 → K1_DRIVE
    GPIO11 → K2_DRIVE
    GPIO12 → K3_DRIVE
    GPIO13 → SSR1_DRIVE
    GPIO14 → SSR2_DRIVE
    GPIO15 → STATUS_LED
    GPIO16 → SPI_MISO (MAX31855 DO)
    GPIO17 → SPI_CS (MAX31855 CS)
    GPIO18 → SPI_SCK (MAX31855 CLK)
    GPIO19 → BUZZER
    GPIO20 → K4_DRIVE
    GPIO21 → WEIGHT_STOP (J15-7, ESP32 brew-by-weight signal)
    GPIO22 → SPARE (J15-8, reserved for future)
    GPIO26 → ADC0_BREW_NTC (J10)
    GPIO27 → ADC1_STEAM_NTC (J11)
    GPIO28 → ADC2_PRESSURE (from J13 voltage divider)
    RUN    → SW1_RESET
    BOOTSEL→ SW2_BOOT

    SENSOR CONNECTOR NETS:
    ──────────────────────
    J10 (Brew NTC)    → NTC_BREW+ to R1, NTC_BREW- to GND
    J11 (Steam NTC)   → NTC_STEAM+ to R2, NTC_STEAM- to GND
    J12 (Thermocouple)→ TC+ to U4 pin 1, TC- to U4 pin 2
    J13 (Pressure 0.5-4.5V) → 5V, GND, SIGNAL to R3/R4 divider

    SOLDER BRIDGE:
    ──────────────
```

---

*End of Schematic Reference Document*

