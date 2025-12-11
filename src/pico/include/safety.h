#ifndef SAFETY_H
#define SAFETY_H

#include <stdint.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Safety States
// -----------------------------------------------------------------------------
typedef enum {
    SAFETY_OK = 0,
    SAFETY_WARNING,
    SAFETY_FAULT,
    SAFETY_CRITICAL
} safety_state_t;

// -----------------------------------------------------------------------------
// Safety Flags
// -----------------------------------------------------------------------------
#define SAFETY_FLAG_OVER_TEMP           (1 << 0)
#define SAFETY_FLAG_WATER_LOW           (1 << 1)
#define SAFETY_FLAG_SENSOR_FAIL         (1 << 2)
#define SAFETY_FLAG_WATCHDOG            (1 << 3)
#define SAFETY_FLAG_COMM_TIMEOUT        (1 << 4)
#define SAFETY_FLAG_ENV_CONFIG_INVALID  (1 << 5)  // Environmental config not set

// -----------------------------------------------------------------------------
// Safe State Definition
// -----------------------------------------------------------------------------
// When entering safe state:
// - All heaters OFF
// - Pump OFF
// - Solenoid OFF (if present)
// - Only manual reset can exit

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

// Initialize safety system (call early in boot)
void safety_init(void);

// Check all safety conditions (call in main loop, returns state)
safety_state_t safety_check(void);

// Get current safety flags
uint8_t safety_get_flags(void);

// Force safe state (emergency shutdown)
void safety_enter_safe_state(void);

// Check if in safe state
bool safety_is_safe_state(void);

// Reset from safe state (requires explicit user action)
bool safety_reset(void);

// Update watchdog (must be called regularly)
void safety_kick_watchdog(void);

// Register heartbeat from ESP32
void safety_esp32_heartbeat(void);

// Check if ESP32 is connected
bool safety_esp32_connected(void);

// Check if in defensive mode (ESP32 not connected, machine forced to IDLE)
bool safety_is_defensive_mode(void);

// Get last alarm code
uint8_t safety_get_last_alarm(void);

#endif // SAFETY_H

