#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

// -----------------------------------------------------------------------------
// Output State
// -----------------------------------------------------------------------------
// Optimized member order (largest to smallest) to minimize padding:
// uint16 (2 bytes) -> uint8 (1 byte)
typedef struct {
    uint16_t power_watts;   // Estimated power draw
    uint8_t brew_heater;     // 0-100%
    uint8_t steam_heater;    // 0-100%
    uint8_t pump;            // 0-100%
    // 1 byte padding may be added by compiler for alignment
    // Total size: 6 bytes (with padding) or 5 bytes (packed)
} control_outputs_t;

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

// Initialize control system
void control_init(void);

// Update control loop (call at CONTROL_LOOP_PERIOD_MS interval)
void control_update(void);

// Setpoint control
void control_set_setpoint(uint8_t target, int16_t temp);  // target: 0=brew, 1=steam
int16_t control_get_setpoint(uint8_t target);

// PID tuning
void control_set_pid(uint8_t target, float kp, float ki, float kd);
void control_get_pid(uint8_t target, float* kp, float* ki, float* kd);

// Output control
void control_set_output(uint8_t output, uint8_t value, uint8_t mode);
void control_get_outputs(control_outputs_t* outputs);

// Pump control
void control_set_pump(uint8_t value);

// Configuration
void control_get_config(config_payload_t* config);
void control_set_config(const config_payload_t* config);

// Heating strategy control
bool control_set_heating_strategy(uint8_t strategy);  // Returns false if strategy not allowed
uint8_t control_get_heating_strategy(void);
bool control_is_heating_strategy_allowed(uint8_t strategy);  // Check if strategy is safe
uint8_t control_get_allowed_strategies(uint8_t* allowed, uint8_t max_count);  // Get list of allowed strategies

// Single boiler mode control (only valid for single boiler machines)
uint8_t control_get_single_boiler_mode(void);  // 0=brew, 1=steam, 2=switching
bool control_is_single_boiler_switching(void); // Returns true during mode switch

#endif // CONTROL_H

