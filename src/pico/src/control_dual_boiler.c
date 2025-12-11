/**
 * ECM Pico Firmware - Dual Boiler Control Implementation
 * 
 * Control logic for dual boiler machines (e.g., ECM Synchronika):
 * - Independent brew and steam boiler PIDs
 * - Multiple heating strategies (sequential, parallel, smart stagger)
 * - Current limiting based on electrical configuration
 */

#include "pico/stdlib.h"
#include "control_impl.h"
#include "config.h"
#include "machine_config.h"
#include <math.h>

// =============================================================================
// Dual Boiler Initialization
// =============================================================================

void control_init_machine(void) {
    // Dual boiler: independent PIDs with standard setpoints
    g_brew_pid.setpoint = TEMP_DECI_TO_C(DEFAULT_BREW_TEMP);
    g_brew_pid.setpoint_target = g_brew_pid.setpoint;
    g_steam_pid.setpoint = TEMP_DECI_TO_C(DEFAULT_STEAM_TEMP);
    g_steam_pid.setpoint_target = g_steam_pid.setpoint;
    
    // Default heating strategy for dual boiler
    g_heating_strategy = HEAT_SEQUENTIAL;
    
    LOG_PRINT("Control: Dual boiler mode initialized\n");
    DEBUG_PRINT("  Brew setpoint: %.1fC\n", g_brew_pid.setpoint);
    DEBUG_PRINT("  Steam setpoint: %.1fC\n", g_steam_pid.setpoint);
}

// =============================================================================
// Dual Boiler Control Update
// =============================================================================

void control_update_machine(
    machine_mode_t mode,
    float brew_temp, float steam_temp, float group_temp,
    float dt,
    float* brew_duty, float* steam_duty
) {
    (void)mode;        // Dual boiler doesn't change behavior based on mode
    (void)group_temp;  // Not used in dual boiler control
    
    // Safety: Handle invalid sensor readings
    float brew_demand = 0.0f;
    float steam_demand = 0.0f;
    
    // Only compute PID if temperature is valid
    if (!isnan(brew_temp) && !isinf(brew_temp)) {
        brew_demand = pid_compute(&g_brew_pid, brew_temp, dt);
    }
    
    if (!isnan(steam_temp) && !isinf(steam_temp)) {
        steam_demand = pid_compute(&g_steam_pid, steam_temp, dt);
    }
    
    // Apply heating strategy
    apply_heating_strategy(brew_demand, steam_demand, brew_temp, steam_temp,
                          brew_duty, steam_duty);
}

// =============================================================================
// Dual Boiler Mode Functions (not applicable)
// =============================================================================

uint8_t control_get_machine_mode(void) {
    return 0;  // Not applicable for dual boiler
}

bool control_is_machine_switching(void) {
    return false;  // Dual boiler doesn't switch modes
}

