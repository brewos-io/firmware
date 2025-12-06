# Hardware Specification Changelog

## Revision History

| Rev    | Date     | Description                                                       |
| ------ | -------- | ----------------------------------------------------------------- |
| 2.22   | Dec 2025 | **CURRENT** - Engineering review fixes (thermal, GPIO protection) |
| 2.21.1 | Dec 2025 | Pico 2 compatibility fixes, power supply                          |
| 2.21   | Dec 2025 | External power metering, multi-machine NTC support                |
| 2.20   | Dec 2025 | Unified 22-pos screw terminal (J26)                               |
| 2.19   | Dec 2025 | Removed spare relay K4                                            |
| 2.17   | Nov 2025 | Brew-by-weight support (J15 8-pin)                                |
| 2.16   | Nov 2025 | Production-ready specification                                    |

---

## v2.22 (December 2025)

**Engineering Design Review Fixes - Thermal & GPIO Protection**

This revision addresses critical issues identified in an independent engineering design qualification and reliability analysis of the RP2350-based control system.

### 🔴 Critical Engineering Change Orders (ECOs)

#### 1. Power Regulator: LDO → Synchronous Buck Converter

| Component | Old (v2.21)      | New (v2.22)       | Reason                                      |
| --------- | ---------------- | ----------------- | ------------------------------------------- |
| **U3**    | AP2112K-3.3 LDO  | TPS563200DDCR     | LDO thermal risk in hot environment         |
| **L1**    | (none)           | 4.7µH inductor    | Required for buck topology                  |
| **C3**    | 100µF Alum       | 22µF Ceramic X5R  | Buck input cap                              |
| **C4**    | 47µF Ceramic     | 22µF Ceramic X5R  | Buck output cap                             |
| **C4A**   | (none)           | 22µF Ceramic X5R  | Parallel output cap for low ripple          |

**Problem:** SOT-23-5 LDO dissipates P = (5V - 3.3V) × I = 1.7W × I. At 250mA load in 60°C ambient, junction temperature exceeds 125°C maximum rating, causing thermal shutdown.

**Solution:** Synchronous buck converter at >90% efficiency reduces heat by 10×. Same footprint complexity, vastly improved thermal reliability.

#### 2. PZEM Interface: 5V→3.3V Level Shifting

| Component | Old (v2.21) | New (v2.22)              | Reason                                 |
| --------- | ----------- | ------------------------ | -------------------------------------- |
| **R45**   | 33Ω series  | 2.2kΩ (upper divider)    | RP2350 GPIO is NOT 5V tolerant         |
| **R45A**  | (none)      | 3.3kΩ (lower divider)    | Creates voltage divider                |
| **R45B**  | (none)      | 33Ω (series after div)   | ESD protection after divider           |

**Problem:** PZEM-004T outputs 5V TTL signals. RP2350 internal protection diodes conduct at ~3.6V, stressing silicon and causing long-term GPIO degradation or latch-up.

**Solution:** Resistive divider: V_out = 5V × 3.3k / (2.2k + 3.3k) = 3.0V (safe for 3.3V GPIO).

#### 3. Relay K1 Function Clarification

| Aspect          | Old (v2.21)               | New (v2.22)                              |
| --------------- | ------------------------- | ---------------------------------------- |
| **Description** | "Water LED relay"         | "Mains indicator lamp relay"             |
| **Load Type**   | Ambiguous                 | Mains indicator lamp (~100mA)            |
| **Relay**       | APAN3105 (3A slim)        | APAN3105 (3A slim) - unchanged           |

**Clarification:** K1 switches a mains-powered indicator lamp on the machine front panel (~100mA load). The 3A slim relay (APAN3105) is appropriate for this low-current application. The relay is required because the indicator is mains-level, but the current draw is minimal.

### 🟡 Reliability Enhancements

#### 4. Fast Flyback Diodes

| Component | Old (v2.21) | New (v2.22) | Reason                                    |
| --------- | ----------- | ----------- | ----------------------------------------- |
| **D1-D3** | 1N4007      | UF4007      | Fast recovery (75ns) for snappy contacts  |

**Improvement:** Standard 1N4007 has slow reverse recovery, causing relay coil current to decay slowly ("lazy" contact opening). UF4007 fast recovery diode dissipates stored energy faster, resulting in snappier relay contact separation and reduced arcing on mains-side contacts.

#### 5. Precision ADC Voltage Reference

| Component | Old (v2.21)                | New (v2.22)                 | Reason                          |
| --------- | -------------------------- | --------------------------- | ------------------------------- |
| **U5**    | (none)                     | LM4040DIM3-3.0              | Precision 3.0V shunt reference  |
| **R7**    | (none)                     | 1kΩ 1% (bias)               | Sets reference operating point  |
| **C7**    | 22µF on 3.3V_A             | 22µF on ADC_VREF            | Bulk cap for reference          |
| **C7A**   | (none)                     | 100nF (HF decoupling)       | Reference noise filtering       |
| **FB1**   | 3.3V → 3.3V_A              | 3.3V → ADC_VREF (via R7)    | Isolates reference from noise   |
| **R1,R2** | Pull-up to 3.3V_A          | Pull-up to ADC_VREF         | Uses precision reference        |
| **TP2**   | (none)                     | ADC_VREF test point         | Calibration verification        |

**Improvement:** Using the 3.3V rail as ADC reference couples regulator thermal drift into temperature measurements. The LM4040 provides a stable 3.0V ±0.5% reference with 100ppm/°C tempco, significantly improving temperature measurement accuracy and stability.

### Component Summary (v2.22 Changes)

| Category       | Added                                              | Changed                              | Removed |
| -------------- | -------------------------------------------------- | ------------------------------------ | ------- |
| **ICs**        | U5 (LM4040DIM3-3.0)                                | U3 (AP2112K → TPS563200)             | -       |
| **Inductors**  | L1 (4.7µH)                                         | -                                    | -       |
| **Resistors**  | R7, R45A, R45B                                     | R45 (33Ω → 2.2kΩ)                    | -       |
| **Capacitors** | C4A, C7A                                           | C3 (100µF → 22µF), C4, C7            | -       |
| **Diodes**     | -                                                  | D1-D3 (1N4007 → UF4007)              | -       |
| **Relays**     | -                                                  | K1 description clarified             | -       |
| **Test Points**| TP2 (ADC_VREF)                                     | -                                    | -       |

### Design Verdict

**Status:** Approved for production (pending verification of changes)

The engineering review identified this design as fundamentally sound with excellent practices in several areas (Wien bridge level sensor, snubber networks). With the implementation of these thermal and logic-level protections, the design is qualified as a high-reliability platform suitable for commercial/prosumer applications.

---

## v2.21 (December 2025)

**Universal External Power Metering & Multi-Machine Support**

### MCU Upgrade: Raspberry Pi Pico 2 (RP2350)

| Change          | Description                                          |
| --------------- | ---------------------------------------------------- |
| **MCU**         | Upgraded from RP2040 to **RP2350** (Pico 2)          |
| **Part Number** | SC0942 (Pico 2) or SC1632 (Pico 2 W with WiFi)       |
| **Performance** | Dual Cortex-M33 @ 150MHz (vs M0+ @ 133MHz)           |
| **Memory**      | 520KB SRAM, 4MB Flash (vs 264KB/2MB)                 |
| **PIO**         | 12 state machines in 3 blocks (vs 8 in 2 blocks)     |
| **ADC**         | 4 channels (vs 3) - extra channel available          |
| **E9 Errata**   | Pull-down resistors on inputs mitigate GPIO latching |

**Note:** Same form factor and pinout - drop-in upgrade from original Pico.

### Power Metering (External Modules)

| Change       | Description                                                 |
| ------------ | ----------------------------------------------------------- |
| **REMOVED**  | Embedded PZEM-004T daughterboard                            |
| **J17 (LV)** | JST-XH 6-pin for UART/RS485 communication                   |
| **J24 (HV)** | Screw terminal 3-pos (L fused, N, PE) for easy meter wiring |
| **U8**       | MAX3485 RS485 transceiver with JP1 termination jumper       |
| **J26**      | Reduced 24→22 pos (CT clamp pins removed)                   |

**Supported meters:** PZEM-004T, JSY-MK-163T/194T, Eastron SDM, and other Modbus meters

### Multi-Machine NTC Support (Jumper Selectable)

| Jumper Config     | Machine Type   | NTC  | Effective R1 | Effective R2 |
| ----------------- | -------------- | ---- | ------------ | ------------ |
| JP2/JP3 **OPEN**  | ECM, Profitec  | 50kΩ | 3.3kΩ        | 1.2kΩ        |
| JP2/JP3 **CLOSE** | Rocket, Gaggia | 10kΩ | ~1kΩ         | ~430Ω        |

**New components:** R1A (1.5kΩ), R2A (680Ω) parallel resistors via solder jumpers

### Expansion & Documentation

- **GPIO22 Expansion:** Available on J15 Pin 8 (SPARE) for future use (flow meter, etc.)
- **J25 REMOVED:** GPIO23 is internal to Pico 2 module (SMPS Power Save) - not on header
- **Section 14.3a:** Solder Jumpers (JP1, JP2, JP3)
- **Section 14.9:** External Sensors BOM with ordering specs
- **Sensor restrictions documented:** Type-K thermocouple only, 0.5-4.5V pressure only
- **C2 upgraded:** 470µF 6.3V Polymer capacitor (low ESR, long life in hot environment)
  - C3 removed - single bulk cap sufficient with HLK internal filtering
- **K1/K3 downsized:** Panasonic APAN3105 (3A, slim 5mm) replaces HF46F (10A)
  - K2 (pump) stays Omron G5LE-1A4 DC5 (16A) for motor inrush
  - Saves ~16mm PCB width
- **Snubbers → MOVs:** Replaced bulky X2 caps (C50-C51) + resistors (R80-R81) with MOVs (RV2-RV3)
  - S10K275 varistors (10mm disc) - ~70% smaller than RC snubbers
  - Critical for slim relay contact protection

### Fixes & Clarifications (v2.21.1)

| Item              | Issue                          | Fix                                                                   |
| ----------------- | ------------------------------ | --------------------------------------------------------------------- |
| **Power Supply**  | HLK-5M05 (1A) insufficient     | Changed to **HLK-15M05C** (3A/15W, 48×28×18mm)                        |
| **GPIO23**        | Not available on Pico 2 header | J25 expansion header **removed** (use GPIO22 via J15 Pin 8)           |
| **3.3V Rail**     | Unclear power architecture     | Clarified: External LDO (U3) for sensors only, Pico has internal 3.3V |
| **Diagram**       | "RELAYS (4x)" incorrect        | Fixed to **"RELAYS (3x)"** (K1, K2, K3)                               |
| **SSR Pins**      | J26 Pin 19-22 referenced       | Fixed: **SSR1=Pin 17-18, SSR2=Pin 19-20**                             |
| **Power Budget**  | Relay current incorrect        | Updated: 80mA typical, 150mA peak (K2:70mA, K1/K3:40mA)               |
| **Total Current** | Old values                     | Updated: **~355mA typical, ~910mA peak** (3A gives 3× headroom)       |

### New Features (v2.21.1)

| Item             | Description                                                                    |
| ---------------- | ------------------------------------------------------------------------------ |
| **ADC Clamping** | Added **D16 (BAT54S)** Schottky diode to protect pressure ADC from overvoltage |

**ADC Protection:** Prevents RP2350 damage if pressure transducer voltage divider fails.

### Pico 2 GPIO Clarifications

**Internal GPIOs (NOT on Pico 2 40-pin header):**

| GPIO   | Internal Function | Notes                    |
| ------ | ----------------- | ------------------------ |
| GPIO23 | SMPS Power Save   | Cannot use for J25       |
| GPIO24 | VBUS Detect       | USB connection sense     |
| GPIO25 | Onboard LED       | Green LED on module      |
| GPIO29 | VSYS/3 ADC        | Internal voltage monitor |

**Available on header:** GPIO 0-22, 26-28 (26 pins total)

---

## v2.20 (December 2025)

**Unified Low-Voltage Terminal Block**

- **J26 unified screw terminal (22-pos):** ALL low-voltage connections consolidated
- **Includes:** Switches (S1-S4), NTCs (T1-T2), Thermocouple, Pressure, SSRs
- **Eliminates:** J10, J11, J12, J13, J18, J19 (all merged into J26)
- **6.3mm spades retained ONLY for 220V AC:** Mains input and relay outputs

---

## v2.19 (December 2025)

**Simplified Relay Configuration**

- Removed spare relay K4 and associated components
- GPIO20 made available as test point TP1

---

## v2.17 (November 2025)

**Brew-by-Weight Support**

- J15 ESP32 connector expanded from 6-pin to 8-pin JST-XH
- GPIO21: WEIGHT_STOP signal (ESP32 → Pico)
- GPIO22: SPARE for future expansion
- Enables Bluetooth scale integration via ESP32

---

## v2.16 (November 2025)

**Production-Ready Specification**

- Power supply: HLK-15M05C (5V 3A/15W, 48×28×18mm)
- Level probe: OPA342 + TLV3201 AC sensing circuit
- Snubbers: MANDATORY for K2 (Pump) and K3 (Solenoid)
- NTC pull-ups: Optimized for 50kΩ NTCs (R1=3.3kΩ, R2=1.2kΩ)
- Mounting: MH1=PE star point (PTH), MH2-4=NPTH (isolated)
- SSR control: 5V trigger signals only, mains via existing machine wiring
