/**
 * ECM Pico Firmware - Single Boiler Control Implementation
 * 
 * Control logic for single boiler machines (e.g., Rancilio Silvia):
 * - Single PID controlling one boiler
 * - Mode switching between brew and steam setpoints
 * - Configurable mode switch delay
 * - Auto-return to brew mode after steam timeout
 */

#include "pico/stdlib.h"
#include "control_impl.h"
#include "config.h"
#include "machine_config.h"
#include <math.h>

// =============================================================================
// Single Boiler Mode State
// =============================================================================

typedef enum {
    SB_MODE_BREW = 0,
    SB_MODE_STEAM = 1,
    SB_MODE_SWITCHING = 2
} single_boiler_mode_t;

static single_boiler_mode_t g_sb_mode = SB_MODE_BREW;
static uint32_t g_sb_mode_switch_time = 0;
static uint32_t g_sb_steam_start_time = 0;

// =============================================================================
// Single Boiler Initialization
// =============================================================================

void control_init_machine(void) {
    const single_boiler_config_t* sb_config = machine_get_single_boiler_config();
    
    // Single boiler: start in brew mode
    g_brew_pid.setpoint = sb_config ? sb_config->brew_setpoint : TEMP_DECI_TO_C(DEFAULT_BREW_TEMP);
    g_brew_pid.setpoint_target = g_brew_pid.setpoint;
    g_steam_pid.setpoint = 0;  // Not used directly
    
    g_sb_mode = SB_MODE_BREW;
    g_heating_strategy = HEAT_BREW_ONLY;
    
    LOG_PRINT("Control: Single boiler mode initialized\n");
    DEBUG_PRINT("  Starting in BREW mode (%.1fC)\n", g_brew_pid.setpoint);
    if (sb_config) {
        DEBUG_PRINT("  Steam setpoint: %.1fC\n", sb_config->steam_setpoint);
        DEBUG_PRINT("  Mode switch delay: %dms\n", sb_config->mode_switch_delay_ms);
        DEBUG_PRINT("  Auto-return: %s (%ds timeout)\n", 
                   sb_config->auto_return_to_brew ? "enabled" : "disabled",
                   sb_config->steam_timeout_s);
    }
}

// =============================================================================
// Mode Switching Logic
// =============================================================================

static void handle_mode_switch(machine_mode_t mode) {
    const single_boiler_config_t* sb_config = machine_get_single_boiler_config();
    if (!sb_config) return;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Check for mode change request
    if (mode == MODE_STEAM && g_sb_mode != SB_MODE_STEAM && g_sb_mode != SB_MODE_SWITCHING) {
        DEBUG_PRINT("Control: Single boiler switching to STEAM mode\n");
        g_sb_mode = SB_MODE_SWITCHING;
        g_sb_mode_switch_time = now;
        g_sb_steam_start_time = now;
        g_brew_pid.setpoint_target = sb_config->steam_setpoint;
        g_brew_pid.setpoint_ramping = true;
    } else if (mode != MODE_STEAM && g_sb_mode == SB_MODE_STEAM) {
        DEBUG_PRINT("Control: Single boiler switching to BREW mode\n");
        g_sb_mode = SB_MODE_SWITCHING;
        g_sb_mode_switch_time = now;
        g_brew_pid.setpoint_target = sb_config->brew_setpoint;
        g_brew_pid.setpoint_ramping = true;
    }
    
    // Check if switching is complete
    if (g_sb_mode == SB_MODE_SWITCHING) {
        if ((now - g_sb_mode_switch_time) >= sb_config->mode_switch_delay_ms) {
            if (mode == MODE_STEAM) {
                g_sb_mode = SB_MODE_STEAM;
                DEBUG_PRINT("Control: Single boiler now in STEAM mode (%.1fC)\n", 
                           sb_config->steam_setpoint);
            } else {
                g_sb_mode = SB_MODE_BREW;
                DEBUG_PRINT("Control: Single boiler now in BREW mode (%.1fC)\n", 
                           sb_config->brew_setpoint);
            }
        }
    }
    
    // Auto-return to brew mode after steam timeout
    if (g_sb_mode == SB_MODE_STEAM && 
        sb_config->auto_return_to_brew && 
        sb_config->steam_timeout_s > 0) {
        uint32_t steam_duration_s = (now - g_sb_steam_start_time) / 1000;
        if (steam_duration_s >= sb_config->steam_timeout_s) {
            DEBUG_PRINT("Control: Single boiler auto-returning to BREW mode (timeout)\n");
            g_sb_mode = SB_MODE_SWITCHING;
            g_sb_mode_switch_time = now;
            g_brew_pid.setpoint_target = sb_config->brew_setpoint;
            g_brew_pid.setpoint_ramping = true;
        }
    }
}

// =============================================================================
// Single Boiler Control Update
// =============================================================================

void control_update_machine(
    machine_mode_t mode,
    float brew_temp, float steam_temp, float group_temp,
    float dt,
    float* brew_duty, float* steam_duty
) {
    (void)steam_temp;  // Single boiler uses brew_temp for the single NTC
    (void)group_temp;
    
    // Handle mode switching
    handle_mode_switch(mode);
    
    // Safety: Only compute PID if temperature is valid
    float demand = 0.0f;
    if (!isnan(brew_temp) && !isinf(brew_temp)) {
        demand = pid_compute(&g_brew_pid, brew_temp, dt);
    }
    
    // Single boiler uses brew SSR only
    *brew_duty = demand;
    *steam_duty = 0.0f;
}

// =============================================================================
// Single Boiler Mode Functions
// =============================================================================

uint8_t control_get_machine_mode(void) {
    return (uint8_t)g_sb_mode;
}

bool control_is_machine_switching(void) {
    return g_sb_mode == SB_MODE_SWITCHING;
}

