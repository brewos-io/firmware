# ECM Synchronika Control Board - Test Procedures

## Pre-Manufacturing Validation & Prototype Testing Guide

**Document Purpose:** Step-by-step test procedures for validating the control board design  
**Revision:** 1.0  
**Date:** November 2025  
**Related Document:** ECM_Control_Board_Specification_v2.md

---

# TABLE OF CONTENTS

1. [Safety Warnings](#1-safety-warnings)
2. [Required Equipment](#2-required-equipment)
3. [Phase 1: Breadboard Testing](#3-phase-1-breadboard-testing-before-pcb-order)
4. [Phase 2: Prototype Board Testing](#4-phase-2-prototype-board-testing)
5. [Phase 3: Machine Integration Testing](#5-phase-3-machine-integration-testing)
6. [Risk Mitigation Checklist](#6-risk-mitigation-checklist)
7. [Test Results Log](#7-test-results-log)

---

# 1. Safety Warnings

```
┌────────────────────────────────────────────────────────────────────────────────┐
│  ⚠️⚠️⚠️  CRITICAL SAFETY WARNINGS  ⚠️⚠️⚠️                                      │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  THIS BOARD OPERATES AT MAINS VOLTAGE (100-240V AC)                           │
│  LETHAL VOLTAGES ARE PRESENT WHEN POWERED                                      │
│                                                                                 │
│  MANDATORY SAFETY RULES:                                                       │
│  ────────────────────────                                                      │
│  1. NEVER work on mains circuits alone - have someone nearby                  │
│  2. ALWAYS use an isolation transformer for initial power-up                  │
│  3. WAIT 30+ seconds after power-off before touching (capacitor discharge)    │
│  4. USE one-hand rule when probing live circuits                              │
│  5. VERIFY power is OFF with multimeter before touching                       │
│  6. KEEP fire extinguisher (CO2 or dry powder) within reach                   │
│  7. WEAR safety glasses during high-power tests                               │
│  8. WORK on non-conductive surface (rubber mat)                               │
│  9. REMOVE metal jewelry (rings, watches, bracelets)                          │
│  10. KNOW location of circuit breaker/emergency shutoff                       │
│                                                                                 │
│  IF IN DOUBT, DO NOT PROCEED - CONSULT A QUALIFIED ELECTRICIAN                │
│                                                                                 │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

# 2. Required Equipment

## 2.1 Essential Test Equipment

| Equipment                  | Purpose                      | Minimum Specs          | Recommended          |
| -------------------------- | ---------------------------- | ---------------------- | -------------------- |
| Digital Multimeter         | Voltage, current, resistance | CAT III 600V rated     | Fluke 117 or similar |
| Isolation Transformer      | Safe mains testing           | 500VA, 1:1 ratio       | With current limiter |
| Variable DC Power Supply   | Circuit testing              | 0-30V, 0-3A            | Dual channel         |
| Oscilloscope               | Signal integrity             | 50MHz, 2 channel       | 100MHz+ with probes  |
| Soldering Station          | Assembly                     | Temperature controlled | 60W+                 |
| Small Flathead Screwdriver | Screw terminal wiring        | 2.5mm blade            | For Phoenix MKDS     |

## 2.2 Recommended Additional Equipment

| Equipment                       | Purpose                  | Notes                        |
| ------------------------------- | ------------------------ | ---------------------------- |
| Thermal Camera / IR Thermometer | Hot spot detection       | Non-contact temp measurement |
| Current Clamp                   | Mains current            | AC clamp meter, 0-20A        |
| ESR Meter                       | Capacitor testing        | For electrolytic caps        |
| LCR Meter                       | Component verification   | Resistance, capacitance      |
| USB Logic Analyzer              | Digital signal debugging | 8+ channels, 24MHz+          |

## 2.3 Test Components & Fixtures

| Item                             | Quantity | Purpose               |
| -------------------------------- | -------- | --------------------- |
| 3.3kΩ 1% resistor                | 5        | NTC simulation        |
| 10kΩ 1% resistor                 | 5        | Various tests         |
| Test leads with clips            | 10       | Connections           |
| Breadboard (830 point)           | 2        | Circuit prototyping   |
| Jumper wires                     | 50       | Connections           |
| K-type thermocouple              | 1        | MAX31855 testing      |
| LED (any color)                  | 5        | Output indication     |
| 1kW resistive load (heater/lamp) | 1        | High current testing  |
| Thermometer (digital)            | 1        | Reference temperature |
| Ice + insulated container        | -        | 0°C reference         |
| Electric kettle / hot water      | -        | High temp reference   |

## 2.4 Safety Equipment

| Item                               | Purpose                |
| ---------------------------------- | ---------------------- |
| Fire extinguisher (CO2/dry powder) | Electrical fire safety |
| Safety glasses                     | Eye protection         |
| Insulated gloves (Class 0)         | Optional, for HV work  |
| Rubber mat                         | Insulated work surface |
| First aid kit                      | Emergency              |

---

# 3. Phase 1: Breadboard Testing (Before PCB Order)

**Objective:** Validate all circuit designs work correctly before committing to PCB fabrication.

---

## 3.1 NTC Thermistor Circuit Test

### Purpose

Verify the voltage divider produces correct output for temperature measurement with 3.3kΩ NTC sensors.

### Circuit to Build

```
        3.3V (from bench supply)
          │
     ┌────┴────┐
     │  3.3kΩ  │  R_series (use 1% tolerance)
     │   1%    │
     └────┬────┘
          │
          ├────────────► V_out (measure here)
          │
     ┌────┴────┐
     │  NTC    │  Your actual ECM NTC sensor
     │  3.3kΩ  │  (or 3.3kΩ 1% resistor to simulate 25°C)
     └────┬────┘
          │
         GND
```

### Test Procedure

| Step | Action                               | Expected Result        | ✓   |
| ---- | ------------------------------------ | ---------------------- | --- |
| 1    | Set bench supply to exactly 3.300V   | Display shows 3.30V    |     |
| 2    | Connect 3.3kΩ 1% resistor as "NTC"   | Simulates 25°C         |     |
| 3    | Measure V_out                        | 1.65V ±0.05V           |     |
| 4    | Replace with actual NTC at room temp | Should match room temp |     |
| 5    | Place NTC in ice water (0°C)         | V_out ≈ 2.45V          |     |
| 6    | Place NTC in hot water (~90°C)       | V_out ≈ 0.37V          |     |
| 7    | Record actual readings below         |                        |     |

### Results Log

| Condition          | Expected V_out | Measured V_out | Expected Temp | Calculated Temp | Error    |
| ------------------ | -------------- | -------------- | ------------- | --------------- | -------- |
| 3.3kΩ resistor     | 1.65V          | \_\_\_V        | 25°C          | N/A             | N/A      |
| Room temp (actual) | ~1.65V         | \_\_\_V        | ~25°C         | \_\_\_°C        | \_\_\_°C |
| Ice water          | 2.45V          | \_\_\_V        | 0°C           | \_\_\_°C        | \_\_\_°C |
| Hot water          | 0.37V          | \_\_\_V        | ~90°C         | \_\_\_°C        | \_\_\_°C |

### Pass Criteria

- [ ] All readings within ±10% of expected voltage
- [ ] Temperature calculation within ±3°C of reference thermometer
- [ ] No erratic readings or noise

### Troubleshooting

| Issue             | Possible Cause        | Solution                          |
| ----------------- | --------------------- | --------------------------------- |
| V_out always 3.3V | NTC open/disconnected | Check NTC continuity              |
| V_out always 0V   | NTC shorted           | Check for shorts                  |
| Noisy readings    | Poor connections      | Use shorter wires, add 100nF cap  |
| Wrong temperature | Incorrect B-value     | Recalculate with sensor datasheet |

---

## 3.2 Relay Driver Circuit Test

### Purpose

Verify the transistor driver can reliably switch relay coils from a 3.3V GPIO signal.

### Circuit to Build

```
                                    +5V (bench supply)
                                      │
                 ┌────────────────────┴────────────────────┐
                 │                                         │
            ┌────┴────┐                               ┌────┴────┐
            │  RELAY  │                               │  470Ω   │  R_led
            │  COIL   │                               └────┬────┘
            │   5V    │                                    │
            └────┬────┘                               ┌────┴────┐
                 │                                    │   LED   │
            ┌────┴────┐                               │  Green  │
            │ 1N4007  │ ← Flyback diode               └────┬────┘
            └────┬────┘                                    │
                 │                                         │
                 └─────────────────────┬───────────────────┤
                                       │                   │
                                  ┌────┴────┐              │
                                  │    C    │              │
                                  │ 2N2222  │              │
    3.3V GPIO ─────────[1kΩ]─────►│    B    │              │
    (or bench supply)             │    E    ├──────────────┘
                      │           └────┬────┘
                 ┌────┴────┐           │
                 │  10kΩ   │          ─┴─
                 │Pull-down│          GND
                 └────┬────┘
                     ─┴─
                     GND
```

**LED current:** (5V - 2.0V) / 470Ω = 6.4mA (clearly visible)

### Test Procedure

| Step | Action                                  | Expected Result           | ✓   |
| ---- | --------------------------------------- | ------------------------- | --- |
| 1    | Build circuit on breadboard             | No shorts                 |     |
| 2    | Set 5V supply, verify voltage           | 5.0V ±0.25V               |     |
| 3    | Leave GPIO input disconnected           | Relay OFF, LED OFF        |     |
| 4    | Connect GPIO input to GND               | Relay OFF, LED OFF        |     |
| 5    | Connect GPIO input to 3.3V              | Relay clicks ON, LED ON   |     |
| 6    | Measure voltage at transistor collector | <0.5V (saturated)         |     |
| 7    | Measure relay coil current              | 70-85mA                   |     |
| 8    | Remove 3.3V from GPIO                   | Relay clicks OFF, LED OFF |     |
| 9    | Rapid on/off cycling (10x)              | Reliable switching        |     |
| 10   | Repeat for all 3 relay types            | All pass                  |     |

### Results Log

| Relay         | Coil Current | Collector Voltage (ON) | Switching Reliable | Pass |
| ------------- | ------------ | ---------------------- | ------------------ | ---- |
| K1 (10A SPST) | \_\_\_mA     | \_\_\_V                | Yes / No           |      |
| K2 (16A SPST) | \_\_\_mA     | \_\_\_V                | Yes / No           |      |
| K3 (10A SPST) | \_\_\_mA     | \_\_\_V                | Yes / No           |      |

### Pass Criteria

- [ ] All relays switch reliably from 3.3V signal
- [ ] Coil current within datasheet specs (typically 70-85mA for 5V relays)
- [ ] Collector voltage < 0.5V when ON (transistor saturated)
- [ ] No chattering or bouncing
- [ ] Pull-down keeps relay OFF when GPIO floating

---

## 3.3 SSR Driver Circuit Test

### Purpose

Verify the SSR trigger circuit provides adequate voltage (~4.8V) for KS15 D-24Z25-LQ SSR (4-32V input).

### Circuit to Build

**IMPORTANT:** SSR has internal current limiting - NO series resistor needed!

```
                                    +5V
                                      │
               ┌──────────────────────┴──────────────────────┐
               │                                             │
               │                                        ┌────┴────┐
               │                                        │  1kΩ    │  R_led
               │                                        └────┬────┘
               │                                             │
               ▼                                        ┌────┴────┐
    ┌──────────────────┐                                │   LED   │  Indicator
    │  SSR Input (+)   │                                │ Orange  │
    └────────┬─────────┘                                └────┬────┘
             │                                               │
             │    ┌───────────────────┐                      │
             │    │   EXTERNAL SSR    │                      │
             │    │   (internal LED   │                      │
             │    │    + resistor)    │                      │
             │    │  KS15 D-24Z25-LQ  │                      │
             │    └───────────────────┘                      │
             │                                               │
    ┌────────┴─────────┐                                     │
    │  SSR Input (-)   ├─────────────────────────────────────┤
    └────────┬─────────┘                                ┌────┴────┐
             │                                          │    C    │
             │                                          │ 2N2222  │
             └─────────────────────────────────────────►│    E    │
                                                        └────┬────┘
                                                             │
    3.3V GPIO ─────────────[1kΩ R_base]─────────────────────►B
                                 │                           │
                            ┌────┴────┐                     ─┴─
                            │  10kΩ   │ Pull-down           GND
                            └────┬────┘
                                ─┴─
                                GND
```

**Operation:**

- GPIO LOW → Transistor OFF → SSR- floating → SSR OFF (no current path)
- GPIO HIGH → Transistor ON → SSR- pulled to GND → SSR ON
- Voltage at SSR = 5V - Vce(sat) = 5V - 0.2V = **4.8V** (exceeds 4V minimum ✓)

### Test Procedure

| Step | Action                              | Expected Result                            | ✓   |
| ---- | ----------------------------------- | ------------------------------------------ | --- |
| 1    | Build circuit without SSR connected |                                            |     |
| 2    | GPIO = GND                          | Transistor OFF, SSR- floating, ~5V at SSR+ |     |
| 3    | GPIO = 3.3V                         | Transistor ON, SSR+ to SSR- = ~4.8V        |     |
| 4    | Connect actual SSR (DC side only)   |                                            |     |
| 5    | GPIO = GND                          | SSR LED OFF                                |     |
| 6    | GPIO = 3.3V                         | SSR LED ON                                 |     |
| 7    | Measure current into SSR            | 8-10mA                                     |     |
| 8    | Check indicator LED                 | LED illuminates                            |     |

### Results Log

| Measurement                                     | Expected | Measured | Pass |
| ----------------------------------------------- | -------- | -------- | ---- |
| SSR+ voltage (from 5V rail)                     | 5.0V     | \_\_\_V  |      |
| Voltage SSR+ to SSR- (GPIO HIGH, SSR connected) | 4.5-4.8V | \_\_\_V  |      |
| Transistor Vce(sat) when ON                     | <0.3V    | \_\_\_V  |      |
| Current into SSR                                | 7-15mA   | \_\_\_mA |      |
| SSR internal LED illuminates                    | Yes      | Yes / No |      |
| Indicator LED illuminates                       | Yes      | Yes / No |      |

### Pass Criteria

- [ ] SSR turns ON reliably when GPIO HIGH (~4.8V across SSR input)
- [ ] SSR turns OFF reliably when GPIO LOW (SSR- floating)
- [ ] Voltage at SSR ≥ 4.0V (meets KS15 D-24Z25-LQ minimum)
- [ ] SSR input current 7-15mA (handled by SSR internal limiting)
- [ ] No excessive heating of transistor or SSR

---

## 3.4 Steam Boiler Level Probe Circuit Test

### Purpose

Verify the level probe sensing circuit works correctly and requires PE-GND connection.

### Circuit to Build

```
        3.3V
          │
     ┌────┴────┐
     │  47kΩ   │  Higher value limits current
     └────┬────┘
          │
          ├──────────────────────► GPIO (measure voltage here)
          │
     ┌────┴────┐     ┌────┴────┐
     │  10kΩ   │     │  100nF  │
     └────┬────┘     └────┬────┘
          │               │
          ├───────────────┤
          │               │
          ▼               │
     "PROBE" ─────────────┤
     (wire)               │
                         ─┴─
                         GND (connected to PE in real machine)
```

### Test Procedure

| Step | Action                                             | Expected Result                | ✓   |
| ---- | -------------------------------------------------- | ------------------------------ | --- |
| 1    | Build circuit on breadboard                        |                                |     |
| 2    | Leave "PROBE" wire disconnected (floating)         | GPIO reads HIGH (3.3V)         |     |
| 3    | Touch "PROBE" wire to GND                          | GPIO reads LOW (0V)            |     |
| 4    | Release "PROBE" wire                               | GPIO returns to HIGH           |     |
| 5    | Simulate water: 1kΩ resistor between PROBE and GND | GPIO reads LOW                 |     |
| 6    | Simulate partial contact: 10kΩ to GND              | GPIO reads LOW (threshold)     |     |
| 7    | Simulate high resistance: 100kΩ to GND             | GPIO reads HIGH (no detection) |     |

### Results Log

| Condition              | Expected GPIO | Measured Voltage | Pass |
| ---------------------- | ------------- | ---------------- | ---- |
| Probe open (air)       | HIGH (3.3V)   | \_\_\_V          |      |
| Probe to GND direct    | LOW (0V)      | \_\_\_V          |      |
| Probe via 1kΩ to GND   | LOW           | \_\_\_V          |      |
| Probe via 10kΩ to GND  | LOW           | \_\_\_V          |      |
| Probe via 100kΩ to GND | HIGH          | \_\_\_V          |      |

### Pass Criteria

- [ ] Open probe reads HIGH (no water detected)
- [ ] Grounded probe reads LOW (water detected)
- [ ] Detection threshold appropriate for water conductivity
- [ ] No oscillation or noise on signal

### Critical Note

```
⚠️  This circuit REQUIRES PE (Protective Earth) to be connected to signal GND.
    In the actual machine, the boiler body is connected to PE.
    Without PE-GND connection, this circuit will NOT work!
```

---

## 3.5 Pressure Transducer Circuit Test (YD4060)

### Purpose

Verify the voltage divider correctly scales the YD4060's 0.5-4.5V output to 0.3-2.7V ADC range.

**Transducer:** YD4060 (0-16 bar, 0.5-4.5V output, 5VDC supply)

### Circuit to Build

```
    Variable Voltage Source               To ADC
    (0.5V to 4.5V)                       (GPIO28)
          │                                  │
          │                                  │
     ┌────┴────┐                             │
     │  10kΩ   │  R3                         │
     │   1%    │                             │
     └────┬────┘                             │
          │                                  │
          ├──────────────────────────────────┤
          │                                  │
     ┌────┴────┐                        ┌────┴────┐
     │  15kΩ   │  R4                    │  100nF  │
     │   1%    │                        │         │
     └────┬────┘                        └────┬────┘
          │                                  │
         GND                                GND
```

### Test Procedure

| Step | Action                           | Expected V_out | ✓   |
| ---- | -------------------------------- | -------------- | --- |
| 1    | Set input to 0.50V               | 0.30V          |     |
| 2    | Set input to 1.00V               | 0.60V          |     |
| 3    | Set input to 2.50V               | 1.50V          |     |
| 4    | Set input to 4.00V               | 2.40V          |     |
| 5    | Set input to 4.50V               | 2.70V          |     |
| 6    | Verify output never exceeds 3.0V | <3.0V always   |     |

### Results Log

| Input Voltage | Expected Output | Measured Output | Ratio | Pass |
| ------------- | --------------- | --------------- | ----- | ---- |
| 0.50V         | 0.30V           | \_\_\_V         | 0.60  |      |
| 1.00V         | 0.60V           | \_\_\_V         | 0.60  |      |
| 2.50V         | 1.50V           | \_\_\_V         | 0.60  |      |
| 4.00V         | 2.40V           | \_\_\_V         | 0.60  |      |
| 4.50V         | 2.70V           | \_\_\_V         | 0.60  |      |

### Pass Criteria

- [ ] Output ratio is 0.60 ±2%
- [ ] Output never exceeds 3.0V (safe for Pico ADC)
- [ ] Stable readings (no noise/drift)

---

## 3.6 MAX31855 Thermocouple Test

### Purpose

Verify SPI communication and temperature reading accuracy.

### Required

- Raspberry Pi Pico (or Pico W)
- MAX31855 breakout board (or MAX31855KASA+ with supporting components)
- K-type thermocouple

### Wiring

| MAX31855 Pin | Pico GPIO | Notes                 |
| ------------ | --------- | --------------------- |
| VCC          | 3V3       | 3.3V power            |
| GND          | GND       | Ground                |
| DO           | GPIO16    | SPI MISO              |
| CS           | GPIO17    | Chip Select           |
| CLK          | GPIO18    | SPI Clock             |
| T+           | TC+       | Thermocouple positive |
| T-           | TC-       | Thermocouple negative |

### Test Firmware (MicroPython)

```python
from machine import Pin, SPI
import time

# Initialize SPI
spi = SPI(0, baudrate=5000000, polarity=0, phase=0,
          sck=Pin(18), mosi=Pin(19), miso=Pin(16))
cs = Pin(17, Pin.OUT)
cs.value(1)

def read_max31855():
    cs.value(0)
    time.sleep_us(1)
    data = spi.read(4)
    cs.value(1)

    # Convert to 32-bit value
    raw = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]

    # Check for faults
    if raw & 0x7:
        fault = raw & 0x7
        if fault & 0x1:
            return "FAULT: Open circuit"
        if fault & 0x2:
            return "FAULT: Short to GND"
        if fault & 0x4:
            return "FAULT: Short to VCC"

    # Extract thermocouple temperature (bits 31-18, signed)
    tc_temp = (raw >> 18) & 0x3FFF
    if tc_temp & 0x2000:  # Negative
        tc_temp = tc_temp - 16384
    tc_temp = tc_temp * 0.25

    # Extract cold junction temperature (bits 15-4, signed)
    cj_temp = (raw >> 4) & 0xFFF
    if cj_temp & 0x800:  # Negative
        cj_temp = cj_temp - 4096
    cj_temp = cj_temp * 0.0625

    return f"TC: {tc_temp:.2f}°C, CJ: {cj_temp:.2f}°C"

# Main loop
print("MAX31855 Thermocouple Test")
print("-" * 40)
while True:
    result = read_max31855()
    print(result)
    time.sleep(1)
```

### Test Procedure

| Step | Action                       | Expected Result                       | ✓   |
| ---- | ---------------------------- | ------------------------------------- | --- |
| 1    | Upload test firmware to Pico | No errors                             |     |
| 2    | Connect MAX31855, no TC      | "FAULT: Open circuit"                 |     |
| 3    | Connect K-type TC            | Temperature reading                   |     |
| 4    | TC at room temperature       | 20-28°C (verify with ref thermometer) |     |
| 5    | TC in ice water              | 0°C ±2°C                              |     |
| 6    | TC in boiling water          | 100°C ±2°C (altitude dependent)       |     |
| 7    | Short TC+ to TC-             | "FAULT: Short to GND/VCC"             |     |
| 8    | Cold junction temp           | Should match room temp                |     |

### Results Log

| Test Condition | Reference Temp | MAX31855 Reading | Error    | Pass |
| -------------- | -------------- | ---------------- | -------- | ---- |
| Room temp      | \_\_\_°C       | \_\_\_°C         | \_\_\_°C |      |
| Ice water      | 0°C            | \_\_\_°C         | \_\_\_°C |      |
| Hot water      | \_\_\_°C       | \_\_\_°C         | \_\_\_°C |      |
| Cold junction  | \_\_\_°C       | \_\_\_°C         | \_\_\_°C |      |

### Pass Criteria

- [ ] SPI communication works (valid readings)
- [ ] Open circuit fault detected correctly
- [ ] Temperature readings within ±2°C of reference
- [ ] No erratic readings or noise

---

## 3.7 ESP32 Interface Test

### Purpose

Verify UART communication and reset/boot control for ESP32 display module.

### Required

- Raspberry Pi Pico
- ESP32 display module (or any ESP32 dev board)
- 2N7002 MOSFETs (2x)
- Resistors per schematic

### Wiring

| Pico GPIO | Function      | ESP32 Pin        |
| --------- | ------------- | ---------------- |
| GPIO0     | UART TX       | RX (via 33Ω)     |
| GPIO1     | UART RX       | TX (via 33Ω)     |
| GPIO21    | Reset Control | EN (via 2N7002)  |
| GPIO22    | Boot Control  | IO0 (via 2N7002) |
| 5V        | Power         | VIN              |
| GND       | Ground        | GND              |

### Test Firmware - Pico Side (MicroPython)

```python
from machine import UART, Pin
import time

# Initialize UART
uart = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1))

# Reset and boot control
rst_pin = Pin(21, Pin.OUT)
boot_pin = Pin(22, Pin.OUT)
rst_pin.value(0)   # LOW = ESP32 running
boot_pin.value(0)  # LOW = normal boot

def reset_esp32():
    """Reset ESP32 into normal run mode"""
    boot_pin.value(0)  # Normal boot
    rst_pin.value(1)   # Assert reset
    time.sleep_ms(100)
    rst_pin.value(0)   # Release reset
    time.sleep_ms(500)
    print("ESP32 reset complete")

def enter_bootloader():
    """Put ESP32 into bootloader mode"""
    boot_pin.value(1)  # Boot mode
    rst_pin.value(1)   # Assert reset
    time.sleep_ms(100)
    rst_pin.value(0)   # Release reset
    time.sleep_ms(100)
    boot_pin.value(0)  # Release boot
    print("ESP32 in bootloader mode")

def uart_test():
    """Test UART communication"""
    print("Sending test message...")
    uart.write("Hello from Pico!\n")
    time.sleep_ms(100)

    if uart.any():
        response = uart.read()
        print(f"Received: {response}")
    else:
        print("No response received")

# Main
print("ESP32 Interface Test")
print("-" * 40)
print("Commands: reset, boot, uart, quit")

while True:
    cmd = input("> ").strip().lower()
    if cmd == "reset":
        reset_esp32()
    elif cmd == "boot":
        enter_bootloader()
    elif cmd == "uart":
        uart_test()
    elif cmd == "quit":
        break
```

### Test Procedure

| Step | Action                                | Expected Result                            | ✓   |
| ---- | ------------------------------------- | ------------------------------------------ | --- |
| 1    | Power on ESP32, observe serial output | Boot messages visible                      |     |
| 2    | Type "reset" command                  | ESP32 reboots, boot messages again         |     |
| 3    | Type "boot" command                   | ESP32 enters bootloader (no boot messages) |     |
| 4    | Connect esptool, verify bootloader    | Chip detected                              |     |
| 5    | Type "reset" to return to normal      | ESP32 boots normally                       |     |
| 6    | Type "uart" - send test message       | Data transmitted                           |     |
| 7    | Verify ESP32 receives message         | Message received                           |     |
| 8    | ESP32 sends response                  | Pico receives response                     |     |

### Results Log

| Test                   | Result      | Notes |
| ---------------------- | ----------- | ----- |
| Reset control          | Pass / Fail |       |
| Bootloader entry       | Pass / Fail |       |
| UART TX (Pico → ESP32) | Pass / Fail |       |
| UART RX (ESP32 → Pico) | Pass / Fail |       |

### Pass Criteria

- [ ] Reset control toggles ESP32 reliably
- [ ] Bootloader mode can be entered
- [ ] UART communication works bidirectionally
- [ ] No data corruption at 115200 baud

---

# 4. Phase 2: Prototype Board Testing

**Objective:** Validate the assembled PCB before connecting to machine.

---

## 4.1 Visual Inspection Checklist

| Item                                 | Check                       | Pass |
| ------------------------------------ | --------------------------- | ---- |
| **Solder Quality**                   |                             |      |
| No solder bridges between IC pins    | Inspect under magnification |      |
| No cold solder joints (dull, grainy) | All joints shiny            |      |
| All SMD components placed correctly  | No tombstoning              |      |
| **Component Orientation**            |                             |      |
| Pico module (if socketed)            | Pin 1 aligned               |      |
| MAX31855 IC                          | Pin 1 dot aligned           |      |
| HLW8012 IC                           | Pin 1 aligned               |      |
| All diodes                           | Cathode band correct        |      |
| Electrolytic capacitors              | Polarity correct            |      |
| LEDs                                 | Polarity correct            |      |
| **High Voltage Section**             |                             |      |
| Isolation slot present               | 2-3mm wide, clean           |      |
| HV traces properly routed            | No crossing to LV           |      |
| Creepage distances                   | ≥6mm HV to LV               |      |
| **Mechanical**                       |                             |      |
| Mounting holes clear                 | No traces blocked           |      |
| Connector alignment                  | Straight, accessible        |      |
| No PCB damage                        | No cracks, delamination     |      |

---

## 4.2 Pre-Power Electrical Tests

### Continuity Tests (Power OFF, No Components Powered)

| Test                      | Probe Points    | Expected        | Measured | Pass |
| ------------------------- | --------------- | --------------- | -------- | ---- |
| 5V to GND (short check)   | VSYS pad to GND | >10kΩ           | \_\_\_Ω  |      |
| 3.3V to GND (short check) | 3V3 pad to GND  | >10kΩ           | \_\_\_Ω  |      |
| L to N (mains short)      | J1-L to J1-N    | Open            | \_\_\_Ω  |      |
| L to GND (isolation)      | J1-L to GND     | Open (>1MΩ)     | \_\_\_Ω  |      |
| N to GND (isolation)      | J1-N to GND     | Open (>1MΩ)     | \_\_\_Ω  |      |
| PE to GND (connected)     | J1-PE to GND    | <1Ω             | \_\_\_Ω  |      |
| Fuse holder               | Across F1       | <0.5Ω (fuse in) | \_\_\_Ω  |      |

### Component Installation Verification

| Component             | Test Method             | Expected         | Pass |
| --------------------- | ----------------------- | ---------------- | ---- |
| All relay coils       | Measure coil resistance | 50-80Ω           |      |
| All LEDs              | Diode test              | 1.8-3.2V forward |      |
| All flyback diodes    | Diode test              | ~0.6V forward    |      |
| Shunt resistor R60    | Low-ohm mode            | 1mΩ ±10%         |      |
| NTC divider resistors | Resistance              | 3.3kΩ ±1%        |      |

---

## 4.3 Low Voltage Power-Up Test

### Setup

1. **DO NOT CONNECT MAINS**
2. Connect external 5V supply to VSYS (via test points or USB)
3. Current-limit supply to 500mA initially

### Test Sequence

| Step | Action                    | Measure               | Expected          | Actual   | Pass |
| ---- | ------------------------- | --------------------- | ----------------- | -------- | ---- |
| 1    | Apply 5V @ 500mA limit    | Supply current        | <100mA            | \_\_\_mA |      |
| 2    | Verify 5V rail            | Voltage at VSYS       | 5.0V ±5%          | \_\_\_V  |      |
| 3    | Verify 3.3V LDO output    | Voltage at 3V3        | 3.3V ±3%          | \_\_\_V  |      |
| 4    | Check for hot components  | Touch test (careful!) | All cool          |          |      |
| 5    | Connect Pico module       | Observe USB LED       | Pico LED flashes  |          |      |
| 6    | Verify USB enumeration    | Computer detects      | "RPI-RP2" or Pico |          |      |
| 7    | Remove USB, power from 5V | 5V current draw       | ~50-100mA         | \_\_\_mA |      |

---

## 4.4 GPIO and Peripheral Test

### Test Firmware

Upload a comprehensive test firmware to exercise all GPIOs:

```python
# gpio_test.py - Upload to Pico
from machine import Pin, ADC, PWM, SPI, UART
import time

# Define all outputs
outputs = {
    10: "K1_RELAY",
    11: "K2_RELAY",
    12: "K3_RELAY",
    13: "SSR1",
    14: "SSR2",
    15: "STATUS_LED",
    19: "BUZZER",
    21: "ESP32_RST",
    22: "ESP32_BOOT",
}

# Define all inputs
inputs = {
    2: "WATER_SW",
    3: "TANK_LVL",
    4: "STEAM_LVL",
    5: "BREW_SW",
}

# ADC channels
adcs = {
    26: "BREW_NTC",
    27: "STEAM_NTC",
    28: "PRESSURE",
}

def test_outputs():
    """Toggle each output"""
    print("\n=== OUTPUT TEST ===")
    for gpio, name in outputs.items():
        pin = Pin(gpio, Pin.OUT)
        print(f"GPIO{gpio} ({name}): ON", end="")
        pin.value(1)
        time.sleep(0.5)
        print(" -> OFF")
        pin.value(0)
        time.sleep(0.2)

def test_inputs():
    """Read all inputs"""
    print("\n=== INPUT TEST ===")
    for gpio, name in inputs.items():
        pin = Pin(gpio, Pin.IN, Pin.PULL_UP)
        value = pin.value()
        state = "HIGH (open)" if value else "LOW (active)"
        print(f"GPIO{gpio} ({name}): {state}")

def test_adc():
    """Read all ADC channels"""
    print("\n=== ADC TEST ===")
    for gpio, name in adcs.items():
        adc = ADC(gpio)
        raw = adc.read_u16()
        voltage = raw * 3.3 / 65535
        print(f"GPIO{gpio} ({name}): {raw} ({voltage:.3f}V)")

def test_buzzer():
    """Play test tone"""
    print("\n=== BUZZER TEST ===")
    pwm = PWM(Pin(19))
    for freq in [1000, 2000, 3000]:
        print(f"  {freq}Hz", end="... ")
        pwm.freq(freq)
        pwm.duty_u16(32768)
        time.sleep(0.3)
        print("done")
    pwm.duty_u16(0)
    pwm.deinit()

def main():
    print("=" * 50)
    print("ECM Control Board - GPIO Test")
    print("=" * 50)

    while True:
        print("\nOptions:")
        print("  1 - Test all outputs (relays, SSRs, LEDs)")
        print("  2 - Read all inputs (switches)")
        print("  3 - Read all ADC channels")
        print("  4 - Buzzer test")
        print("  5 - Run all tests")
        print("  q - Quit")

        cmd = input("\nSelect: ").strip()

        if cmd == "1":
            test_outputs()
        elif cmd == "2":
            test_inputs()
        elif cmd == "3":
            test_adc()
        elif cmd == "4":
            test_buzzer()
        elif cmd == "5":
            test_outputs()
            test_inputs()
            test_adc()
            test_buzzer()
        elif cmd.lower() == "q":
            break

if __name__ == "__main__":
    main()
```

### Output Test Results

| GPIO | Function       | Click/LED Observed | Pass |
| ---- | -------------- | ------------------ | ---- |
| 10   | K1 Relay + LED | Yes / No           |      |
| 11   | K2 Relay + LED | Yes / No           |      |
| 12   | K3 Relay + LED | Yes / No           |      |
| 13   | SSR1 LED       | Yes / No           |      |
| 14   | SSR2 LED       | Yes / No           |      |
| 15   | Status LED     | Yes / No           |      |
| 19   | Buzzer         | Audible / No       |      |

### Input Test Results

| GPIO | Function     | Open State | Grounded State | Pass |
| ---- | ------------ | ---------- | -------------- | ---- |
| 2    | Water Switch | HIGH / LOW | HIGH / LOW     |      |
| 3    | Tank Level   | HIGH / LOW | HIGH / LOW     |      |
| 4    | Steam Level  | HIGH / LOW | HIGH / LOW     |      |
| 5    | Brew Switch  | HIGH / LOW | HIGH / LOW     |      |

### ADC Test Results

| GPIO | Function  | No Input | 3.3kΩ to GND | Pass |
| ---- | --------- | -------- | ------------ | ---- |
| 26   | Brew NTC  | \_\_\_V  | ~1.65V       |      |
| 27   | Steam NTC | \_\_\_V  | ~1.65V       |      |
| 28   | Pressure  | \_\_\_V  | (varies)     |      |

---

## 4.5 Mains Power-Up Test

```
⚠️⚠️⚠️  DANGER - LETHAL VOLTAGES  ⚠️⚠️⚠️

ONLY proceed if:
- You have appropriate electrical safety training
- An isolation transformer is available
- Another person is present
- Fire extinguisher is within reach
```

### Setup

1. Disconnect all loads (relays disconnected from machine)
2. Connect isolation transformer (if available)
3. Connect power via switch-mode power strip with switch
4. Have multimeter ready to measure

### Test Sequence

| Step | Action                   | Measure          | Expected         | Actual  | Pass |
| ---- | ------------------------ | ---------------- | ---------------- | ------- | ---- |
| 1    | Apply mains (via switch) | None             | No sparks/smoke  |         |      |
| 2    | Wait 5 seconds           | Observe          | No heating       |         |      |
| 3    | Measure 5V rail          | Voltage          | 5.0V ±5%         | \_\_\_V |      |
| 4    | Measure idle current     | Clamp meter      | <0.1A            | \_\_\_A |      |
| 5    | Energize one relay (K1)  | Current increase | +~0.08A          | \_\_\_A |      |
| 6    | Energize all 3 relays    | Total current    | ~0.3A            | \_\_\_A |      |
| 7    | Run for 5 minutes        | Temperature      | All cool         |         |      |
| 8    | Power off, wait 30s      |                  | Caps discharged  |         |      |
| 9    | Touch test (careful!)    | Temperature      | Slightly warm OK |         |      |

---

## 4.6 High Current Test

### Setup

- Connect a 1kW resistive load (e.g., heater, incandescent bulbs) through relay K2
- Use clamp meter to measure current
- Have thermal camera or IR thermometer ready

### Test Sequence (⚠️ MAINS VOLTAGE)

| Step | Action                      | Measure        | Expected        | Actual   | Pass |
| ---- | --------------------------- | -------------- | --------------- | -------- | ---- |
| 1    | Energize K2 relay           | Load turns on  | Load operates   |          |      |
| 2    | Measure load current        | Clamp meter    | 4-5A (1kW load) | \_\_\_A  |      |
| 3    | Run for 10 minutes          |                | Continuous      |          |      |
| 4    | Measure relay temperature   | IR thermometer | <60°C           | \_\_\_°C |      |
| 5    | Measure shunt temperature   | IR thermometer | <50°C           | \_\_\_°C |      |
| 6    | Measure fuse holder temp    | IR thermometer | <50°C           | \_\_\_°C |      |
| 7    | Measure spade terminal temp | IR thermometer | <50°C           | \_\_\_°C |      |
| 8    | De-energize K2, power off   |                |                 |          |      |

### Pass Criteria

- [ ] No component exceeds 70°C
- [ ] No discoloration of PCB
- [ ] No burning smell
- [ ] Relay contacts don't weld

---

# 5. Phase 3: Machine Integration Testing

**Objective:** Validate full system operation in the actual espresso machine.

---

## 5.1 Pre-Connection Checklist

| Item                                             | Verification               | Pass |
| ------------------------------------------------ | -------------------------- | ---- |
| Machine unplugged                                | Verified                   |      |
| Original control board removed                   | Documented wire positions  |      |
| All machine wires labeled                        | Photos taken               |      |
| New board PE connected                           | Continuity to chassis      |      |
| **LV Wiring:** Wires stripped 6mm or ferrules    | Visual (J26 - all 24 pins) |      |
| **HV Wiring:** Spade connectors crimped properly | Tug test (J1, J2, J3, J4)  |      |

---

## 5.2 Sensor Connection Test (Machine OFF)

Connect all sensors but do NOT connect mains loads. **All sensors connect to unified J26 terminal block.**

| Sensor            | J26 Pin(s) | Expected Reading     | Actual    | Pass |
| ----------------- | ---------- | -------------------- | --------- | ---- |
| Water Reservoir   | 1, 2       | HIGH (tank removed)  |           |      |
| Tank Level        | 3, 4       | (depends on float)   |           |      |
| Steam Level Probe | 5          | (depends on water)\* |           |      |
| Brew Handle       | 6, 7       | HIGH (handle down)   |           |      |
| Brew NTC          | 8, 9       | Room temp (~25°C)    | \_\_\_°C  |      |
| Steam NTC         | 10, 11     | Room temp (~25°C)    | \_\_\_°C  |      |
| Thermocouple      | 12, 13     | Room temp (~25°C)    | \_\_\_°C  |      |
| Pressure          | 14, 15, 16 | 0 bar                | \_\_\_bar |      |
| CT Clamp          | 17, 18     | (from PZEM J24)      |           |      |
| SSR1 Control      | 19, 20     | 5V output            |           |      |
| SSR2 Control      | 21, 22     | 5V output            |           |      |

> **Note for Steam Level Probe:** Single-wire connection on J26 Pin 5. Return path is via boiler chassis (PE/Earth connection required).

---

## 5.3 Actuator Connection Test (Machine OFF, Mains Disconnected from Loads)

| Actuator      | Connector     | Test Method   | Expected    | Pass |
| ------------- | ------------- | ------------- | ----------- | ---- |
| K1 - LED      | J2 (spade)    | Energize K1   | Click heard |      |
| K2 - Pump     | J3 (spade)    | Energize K2   | Click heard |      |
| K3 - Solenoid | J4 (spade)    | Energize K3   | Click heard |      |
| SSR1 - Brew   | J26 Pin 19-20 | Energize SSR1 | SSR LED on  |      |
| SSR2 - Steam  | J26 Pin 21-22 | Energize SSR2 | SSR LED on  |      |

---

## 5.4 Full Power Integration Test

### Sequence

| Step | Action                   | Verify                       | Pass |
| ---- | ------------------------ | ---------------------------- | ---- |
| 1    | Connect all loads        | Wiring correct               |      |
| 2    | Apply mains power        | No issues                    |      |
| 3    | Pump test (K2)           | Pump runs, water flows       |      |
| 4    | Solenoid test (K3)       | Valve clicks                 |      |
| 5    | Brew heater test (SSR1)  | Element heats (monitor temp) |      |
| 6    | Steam heater test (SSR2) | Element heats (monitor temp) |      |
| 7    | Safety interlock test    | Remove water → heaters OFF   |      |
| 8    | Temperature control test | PID holds setpoint           |      |
| 9    | Full brew cycle          | All systems operational      |      |

---

## 5.5 Safety Interlock Tests

| Test                    | Procedure          | Expected Result       | Actual | Pass |
| ----------------------- | ------------------ | --------------------- | ------ | ---- |
| Water reservoir removed | Remove tank        | Heaters disabled      |        |      |
| Tank level low          | Lower float        | Heaters disabled      |        |      |
| Steam boiler dry        | Simulate low level | Steam heater disabled |        |      |
| Over-temperature        | (test in firmware) | Heaters disabled      |        |      |
| Watchdog timeout        | (test in firmware) | System resets         |        |      |

---

# 6. Risk Mitigation Checklist

## High Voltage Safety

| Risk                   | Mitigation                 | Verified |
| ---------------------- | -------------------------- | -------- |
| Insufficient creepage  | Measure: ≥6mm HV to LV     |          |
| Trace width inadequate | Verify: ≥3mm for 16A paths |          |
| Poor isolation         | Slot present between HV/LV |          |
| Fuse undersized        | Verify: 20A rating         |          |
| No surge protection    | MOV installed              |          |
| Contact arcing         | Flyback diodes installed   |          |

## Thermal

| Risk              | Mitigation             | Verified |
| ----------------- | ---------------------- | -------- |
| Shunt overheating | 5W rating (20× margin) |          |
| Relay coil heat   | Spacing adequate       |          |
| AC/DC module heat | Ventilation space      |          |
| LDO overheating   | Thermal vias present   |          |

## Signal Integrity

| Risk             | Mitigation                     | Verified |
| ---------------- | ------------------------------ | -------- |
| ADC noise        | Analog ground isolation        |          |
| Thermocouple EMI | Traces away from relays        |          |
| UART corruption  | Series resistors installed     |          |
| ESD damage       | ESD clamps on external signals |          |

---

# 7. Test Results Log

## Summary

| Phase                    | Date           | Tester     | Result      |
| ------------------------ | -------------- | ---------- | ----------- |
| 1.1 NTC Test             | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 1.2 Relay Driver         | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 1.3 SSR Driver           | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 1.4 Level Probe          | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 1.5 Pressure             | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 1.6 Thermocouple         | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 1.7 ESP32 Interface      | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 2.1 Visual Inspection    | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 2.2 Pre-Power Tests      | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 2.3 Low Voltage Power-Up | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 2.4 GPIO Test            | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 2.5 Mains Power-Up       | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 2.6 High Current Test    | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |
| 3.1-3.5 Integration      | **_/_**/\_\_\_ | **\_\_\_** | Pass / Fail |

## Issues Found

| Issue # | Description | Severity     | Resolution | Status      |
| ------- | ----------- | ------------ | ---------- | ----------- |
| 1       |             | High/Med/Low |            | Open/Closed |
| 2       |             | High/Med/Low |            | Open/Closed |
| 3       |             | High/Med/Low |            | Open/Closed |

## Sign-Off

| Role     | Name | Signature | Date |
| -------- | ---- | --------- | ---- |
| Tester   |      |           |      |
| Reviewer |      |           |      |

---

_End of Test Procedures Document_
