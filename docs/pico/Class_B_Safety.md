# Class B Safety Routines (IEC 60730/60335)

## Overview

BrewOS implements IEC 60730/60335 Annex R Class B safety self-test routines on the Pico (RP2350) firmware. These routines detect hardware and software faults that could lead to unsafe operation.

> **IMPORTANT DISCLAIMER**: This implementation provides Class B safety self-tests following IEC 60730 Annex R guidelines, but has **NOT been certified** by an accredited test laboratory (TÜV, UL, etc.). For safety-critical production use in commercial products, formal certification is required.

## Compliance Status

| Requirement | Status | Notes |
|-------------|--------|-------|
| RAM Test | ✅ Implemented | March C- algorithm |
| Flash CRC | ✅ Implemented | CRC-32 verification |
| CPU Register Test | ✅ Implemented | Pattern write/read/verify |
| I/O Verification | ✅ Implemented | GPIO read-back |
| Clock Test | ✅ Implemented | Frequency tolerance check |
| Stack Overflow | ✅ Implemented | Canary-based detection |
| Program Counter | ✅ Implemented | Flow verification |
| Certified | ❌ Not certified | Requires TÜV/UL certification |

## Test Descriptions

### RAM Test (March C- Algorithm)

Tests RAM integrity using the March C- algorithm, which detects:
- Stuck-at faults (bits stuck at 0 or 1)
- Transition faults (bits that can't change state)
- Coupling faults (bits affected by neighboring bits)

**Algorithm:**
1. Write 0 to all cells ascending
2. Read 0, write 1 ascending
3. Read 1, write 0 ascending
4. Read 0, write 1 descending
5. Read 1, write 0 descending
6. Read 0 ascending (verify)

**Patterns tested:** `0x00000000`, `0xFFFFFFFF`, `0xAAAAAAAA`, `0x55555555`

**Test region:** Dedicated 64-byte buffer to avoid corrupting application data.

### Flash CRC Verification

Verifies application code integrity by calculating CRC-32 and comparing against a reference value stored at boot.

**Configuration:**
- Flash start: `0x10000000` (XIP base)
- Flash size: 256KB
- Polynomial: CRC-32 (0xEDB88320, reflected)
- Reference calculated at boot

**Incremental calculation:** Flash CRC is calculated incrementally (4KB chunks) during periodic tests to avoid blocking the real-time control loop.

### CPU Register Test

Tests core CPU registers using pattern write/read/verify operations.

**Registers tested:**
- Memory operations (via volatile variables)
- Arithmetic operations (ADD, MUL)
- Pattern verification

**Patterns:** `0x00000000`, `0xFFFFFFFF`, `0xAAAAAAAA`, `0x55555555`, `0x12345678`, `0x87654321`

### I/O State Verification

Verifies GPIO output states match expected values by reading back output registers.

**Outputs verified:**
- SSR outputs (brew, steam)
- Relay outputs (pump, solenoid, water LED)

**Shadow state:** The firmware maintains a shadow copy of expected GPIO states for comparison.

### Clock Frequency Test

Verifies system clock frequency is within tolerance.

**Configuration:**
- Nominal frequency: 125 MHz
- Tolerance: ±5%
- Acceptable range: 118.75 MHz - 131.25 MHz

### Stack Overflow Detection

Uses canary values placed at stack boundaries to detect overflow.

**Configuration:**
- Canary value: `0xDEADBEEF`
- Canaries placed at top and bottom of stack
- Checked every control loop cycle

### Program Counter Test

Verifies program execution flow using sequential function calls with marker verification.

**Method:**
1. Call function 1 → sets marker to value A
2. Call function 2 → sets marker to value B (only if A was set)
3. Call function 3 → sets marker to value C (only if B was set)
4. Verify final marker value is C

## Test Timing

### Startup Tests

Run once at boot before entering the main control loop:
- CPU Register Test
- RAM Test (full)
- Clock Test
- Stack Test
- Program Counter Test
- Flash CRC (reference calculation)

**Duration:** ~200ms

### Periodic Tests

Run during operation, staggered across control loop cycles (10Hz = 100ms per cycle):

| Test | Interval | Duration per cycle |
|------|----------|-------------------|
| RAM | 1 second | ~1ms |
| CPU | 1 second | ~100µs |
| I/O | 1 second | ~50µs |
| Stack | Every cycle | ~10µs |
| Clock | 10 seconds | ~50µs |
| Flash CRC | Incremental | ~100µs per chunk |

**Flash CRC completion:** Full verification completes every ~60 seconds.

## Integration

### Initialization (in `main.c`)

```c
// Initialize Class B safety routines
if (class_b_init() != CLASS_B_PASS) {
    DEBUG_PRINT("ERROR: Class B initialization failed!\n");
}

// Run startup self-test
class_b_result_t result = class_b_startup_test();
if (result != CLASS_B_PASS) {
    safety_enter_safe_state();  // Enter safe state on failure
}
```

### Periodic Testing (in main control loop)

```c
// Run periodic Class B tests
class_b_result_t result = class_b_periodic_test();
if (result != CLASS_B_PASS) {
    safety_enter_safe_state();  // Enter safe state on failure
    protocol_send_alarm(ALARM_WATCHDOG, 2, 0);
}
```

## Failure Response

When any Class B test fails:

1. **Immediate safe state entry:**
   - All heaters OFF
   - Pump OFF
   - Solenoid OFF

2. **Alarm notification:**
   - `ALARM_WATCHDOG` sent to ESP32
   - Debug message logged

3. **Recovery:**
   - Requires explicit reset via `class_b_reset()`
   - Reset only succeeds if self-tests pass
   - Typically requires power cycle

## Diagnostic Commands

Class B tests can be triggered manually via the protocol diagnostic commands:

| Test ID | Name | Description |
|---------|------|-------------|
| `0x30` | `DIAG_TEST_CLASS_B_ALL` | Run all Class B tests |
| `0x31` | `DIAG_TEST_CLASS_B_RAM` | RAM March C- test |
| `0x32` | `DIAG_TEST_CLASS_B_FLASH` | Flash CRC verification |
| `0x33` | `DIAG_TEST_CLASS_B_CPU` | CPU register test |
| `0x34` | `DIAG_TEST_CLASS_B_IO` | I/O state verification |
| `0x35` | `DIAG_TEST_CLASS_B_CLOCK` | Clock frequency test |
| `0x36` | `DIAG_TEST_CLASS_B_STACK` | Stack overflow detection |
| `0x37` | `DIAG_TEST_CLASS_B_PC` | Program counter test |

## For Hobbyist/Non-Certified Use

This implementation provides significant safety margin for DIY projects:

1. **Detects common hardware failures:**
   - RAM bit errors (cosmic rays, voltage issues)
   - Flash corruption (incomplete writes, wear)
   - Clock drift (crystal failure)
   - Stack overflow (software bugs)

2. **Immediate response:**
   - Heaters disabled within 100ms of fault detection
   - No manual intervention required

3. **Monitoring:**
   - Failure counts tracked
   - Diagnostic commands available for testing

**Limitations for hobbyist use:**
- No formal certification
- Self-test coverage may not be 100%
- Hardware watchdog is the ultimate safety net

## For Production/Commercial Use

For commercial products or safety-critical applications:

1. **Formal certification required:**
   - Submit to TÜV, UL, or equivalent laboratory
   - Follow complete IEC 60730 certification process
   - Document all safety analysis (FMEA)

2. **Additional requirements:**
   - Independent hardware watchdog
   - Redundant temperature sensing
   - EMC compliance testing
   - Production test procedures

3. **Consider:**
   - Using a certified Class B library (e.g., STM32 Class B library)
   - Hardware safety features (thermal fuses, OTP sensors)
   - Third-party safety controller

## Files

- `src/pico/include/class_b.h` - Class B API header
- `src/pico/src/class_b.c` - Class B implementation
- `src/shared/protocol_defs.h` - Diagnostic test IDs

## References

- IEC 60730-1: Automatic electrical controls for household and similar use
- IEC 60335-1: Household and similar electrical appliances - Safety
- Annex R: Software assessment (Class B requirements)
- ARM Cortex-M0+ Technical Reference Manual

