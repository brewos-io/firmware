# Hardware Specification Changelog

## Revision History

| Rev    | Date     | Description                                            |
| ------ | -------- | ------------------------------------------------------ |
| 2.21.1 | Dec 2025 | **CURRENT** - Pico 2 compatibility fixes, power supply |
| 2.21   | Dec 2025 | External power metering, multi-machine NTC support     |
| 2.20   | Dec 2025 | Unified 22-pos screw terminal (J26)                    |
| 2.19   | Dec 2025 | Removed spare relay K4                                 |
| 2.17   | Nov 2025 | Brew-by-weight support (J15 8-pin)                     |
| 2.16   | Nov 2025 | Production-ready specification                         |

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
| **SSR Pins**      | J26 Pin 19-22 referenced       | Fixed: **SSR1=Pin 17-18, SSR2=Pin 19-20** (21-22 are spare GND)       |
| **Power Budget**  | Relay current incorrect        | Updated: 80mA typical, 150mA peak (K2:70mA, K1/K3:40mA)               |
| **Total Current** | Old values                     | Updated: **~355mA typical, ~910mA peak** (3A gives 3× headroom)       |

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
