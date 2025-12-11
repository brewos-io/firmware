/**
 * ECM Pico Firmware - Water Management
 * 
 * Handles steam boiler auto-fill and water LED control.
 * 
 * HYDRAULIC SYSTEM:
 * - Steam Boiler: Auto-fill via level probe (GPIO4) and fill solenoid (Path B)
 * - Brew Boiler: Displacement system (always 100% full, no active filling)
 * - Brew Priority: Steam fill is blocked during brewing to prevent pressure drop
 */

#include "pico/stdlib.h"
#include "water_management.h"
#include "config.h"
#include "sensors.h"
#include "hardware.h"
#include "pcb_config.h"
#include "machine_config.h"
#include "safety.h"
#include "control.h"
#include "state.h"
#include "protocol.h"

// =============================================================================
// Configuration
// =============================================================================

#define STEAM_FILL_TIMEOUT_MS        30000   // 30s max fill time (safety)
#define STEAM_LEVEL_DEBOUNCE_MS      100     // Debounce for level probe
#define STEAM_FILL_CHECK_INTERVAL_MS 100     // Check every 100ms
#define STEAM_HEATER_OFF_DELAY_MS    500     // Delay after cutting heater before filling

// =============================================================================
// Steam Boiler Fill States
// =============================================================================

typedef enum {
    STEAM_FILL_IDLE = 0,           // Not filling
    STEAM_FILL_HEATER_OFF,         // Heater cut, waiting before fill
    STEAM_FILL_ACTIVE,             // Actively filling
    STEAM_FILL_COMPLETE             // Fill complete, restoring heater
} steam_fill_state_t;

// =============================================================================
// Private State
// =============================================================================

static bool g_auto_fill_enabled = true;  // Enabled by default for steam boiler
static steam_fill_state_t g_fill_state = STEAM_FILL_IDLE;
static uint32_t g_fill_start_time = 0;
static uint32_t g_heater_off_time = 0;
static uint32_t g_last_fill_check = 0;
static bool g_steam_heater_was_on = false;  // Remember if heater was on before fill

// Level probe debounce
static uint32_t g_steam_level_last_change = 0;
static bool g_steam_level_last_state = false;
static bool g_steam_level_debounced = false;

// Fill cycle monitoring (moved from function scope for proper state management)
static uint32_t g_fill_level_check_time = 0;       // Last time we checked level state
static bool g_fill_level_last_state = false;       // Last level state during fill monitoring

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * Check if steam boiler level is low (with debouncing)
 * Returns true if water is below probe (needs filling)
 * 
 * Logic: GPIO4 HIGH = water below probe (needs fill)
 *        GPIO4 LOW = water at/above probe (OK)
 */
static bool is_steam_level_low(void) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || !PIN_VALID(pcb->pins.input_steam_level)) {
        return false;  // Not configured
    }
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool current_state = hw_read_gpio(pcb->pins.input_steam_level);  // HIGH = low water
    
    if (current_state != g_steam_level_last_state) {
        // State changed, reset debounce timer
        g_steam_level_last_change = now;
        g_steam_level_last_state = current_state;
    }
    
    // Check if debounced
    if (now - g_steam_level_last_change >= STEAM_LEVEL_DEBOUNCE_MS) {
        g_steam_level_debounced = current_state;
    }
    
    return g_steam_level_debounced;
}

/**
 * Read water mode switch (physical switch)
 * Returns true if plumbed mode (switch HIGH), false if water tank mode (switch LOW)
 * Defaults to water tank mode if switch not configured
 */
static bool read_water_mode_switch(void) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || !PIN_VALID(pcb->pins.input_water_mode)) {
        // Switch not configured - default to water tank mode (safe default)
        return false;
    }
    
    // Read switch: HIGH = plumbed mode, LOW = water tank mode
    // Active HIGH (pull-down): HIGH = plumbed, LOW = water tank
    return hw_read_gpio(pcb->pins.input_water_mode);
}

/**
 * Check if machine is in plumbed mode (from physical switch)
 */
static bool is_plumbed_mode(void) {
    return read_water_mode_switch();
}

/**
 * Check if machine is in water tank mode (from physical switch)
 */
static bool is_water_tank_mode(void) {
    return !read_water_mode_switch();
}

/**
 * Check if reservoir is present (only valid in water tank mode)
 * In plumbed mode, always returns true (water line is always available)
 */
static bool is_reservoir_present(void) {
    // In plumbed mode, water is always available from water line
    if (is_plumbed_mode()) {
        return true;
    }
    
    // In water tank mode, check reservoir sensor
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || !PIN_VALID(pcb->pins.input_reservoir)) {
        return true;  // Assume present if not configured (safe default)
    }
    
    // Active LOW: LOW = present, HIGH = empty
    return !hw_read_gpio(pcb->pins.input_reservoir);
}

/**
 * Check if currently brewing (brew priority check)
 */
static bool is_brewing(void) {
    return state_is_brewing();
}

/**
 * Control steam fill solenoid (Path B - steam boiler path)
 */
static void set_steam_fill_solenoid(bool on) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || !PIN_VALID(pcb->pins.relay_fill_solenoid)) {
        return;  // Not configured
    }
    
    hw_set_gpio(pcb->pins.relay_fill_solenoid, on);
    
    if (on) {
        DEBUG_PRINT("Water: Steam fill solenoid ON (Path B open)\n");
    } else {
        DEBUG_PRINT("Water: Steam fill solenoid OFF (Path B closed)\n");
    }
}

/**
 * Control water LED (FUNC-032)
 * Water Tank Mode: LED ON when reservoir present AND tank level OK
 * Plumbed Mode: LED ON when tank level OK (water line always available)
 */
static void update_water_led(void) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || !PIN_VALID(pcb->pins.relay_water_led)) {
        return;  // Not configured
    }
    
    bool led_on = false;
    
    if (is_plumbed_mode()) {
        // Plumbed mode: water line is always available, only check tank level
        bool tank_level_ok = true;
        if (PIN_VALID(pcb->pins.input_tank_level)) {
            bool tank_low = !hw_read_gpio(pcb->pins.input_tank_level);  // Active LOW
            tank_level_ok = !tank_low;
        }
        led_on = tank_level_ok;  // LED ON when tank level OK
    } else {
        // Water tank mode: check both reservoir and tank level
        bool reservoir_ok = is_reservoir_present();
        
        // Check tank level (if configured)
        bool tank_level_ok = true;
        if (PIN_VALID(pcb->pins.input_tank_level)) {
            bool tank_low = !hw_read_gpio(pcb->pins.input_tank_level);  // Active LOW
            tank_level_ok = !tank_low;
        }
        
        // LED ON when reservoir present AND tank level OK
        led_on = reservoir_ok && tank_level_ok;
    }
    
    hw_set_gpio(pcb->pins.relay_water_led, led_on);
}

// =============================================================================
// Initialization
// =============================================================================

void water_management_init(void) {
    g_auto_fill_enabled = true;  // Enabled by default
    g_fill_state = STEAM_FILL_IDLE;
    g_fill_start_time = 0;
    g_heater_off_time = 0;
    g_last_fill_check = 0;
    g_steam_heater_was_on = false;
    
    // Initialize debounce state
    g_steam_level_last_change = 0;
    g_steam_level_last_state = false;
    g_steam_level_debounced = false;
    
    // Initialize fill monitoring state
    g_fill_level_check_time = 0;
    g_fill_level_last_state = false;
    
    // Initialize steam fill solenoid GPIO (if configured)
    const pcb_config_t* pcb = pcb_config_get();
    if (pcb && PIN_VALID(pcb->pins.relay_fill_solenoid)) {
        hw_gpio_init_output(pcb->pins.relay_fill_solenoid, false);  // OFF initially (Path B closed)
        DEBUG_PRINT("Water: Steam fill solenoid initialized (Path B)\n");
    }
    
    // Initialize water LED GPIO (if configured)
    if (pcb && PIN_VALID(pcb->pins.relay_water_led)) {
        hw_gpio_init_output(pcb->pins.relay_water_led, false);  // OFF initially
        DEBUG_PRINT("Water: Water LED initialized\n");
    }
    
    LOG_PRINT("Water: Initialized (steam boiler auto-fill: %s)\n",
              g_auto_fill_enabled ? "enabled" : "disabled");
}

// =============================================================================
// Steam Boiler Fill Cycle
// =============================================================================

/**
 * Steam boiler fill cycle state machine
 * 
 * Sequence:
 * 1. Detect low water (probe HIGH/open circuit)
 * 2. CRITICAL: Cut steam heater FIRST
 * 3. Wait brief delay (STEAM_HEATER_OFF_DELAY_MS)
 * 4. Open fill solenoid (Path B)
 * 5. Turn on pump
 * 6. Fill until probe touches (probe goes LOW)
 * 7. Turn off pump
 * 8. Close solenoid
 * 9. Restore steam heater
 */
static void update_steam_fill_cycle(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool steam_level_low = is_steam_level_low();
    bool brewing = is_brewing();
    
    // BREW PRIORITY: If brewing, ignore steam fill request
    if (brewing && g_fill_state == STEAM_FILL_IDLE) {
        // Don't start fill cycle while brewing
        return;
    }
    
    // If brewing and already filling, stop fill cycle immediately
    if (brewing && g_fill_state != STEAM_FILL_IDLE) {
        DEBUG_PRINT("Water: Brew priority - stopping steam fill cycle\n");
        // Stop fill cycle
        set_steam_fill_solenoid(false);
        control_set_pump(0);
        g_fill_state = STEAM_FILL_IDLE;
        return;
    }
    
    switch (g_fill_state) {
        case STEAM_FILL_IDLE: {
            // Check if we need to start fill cycle
            // In plumbed mode: water line always available, no reservoir check needed
            // In water tank mode: check reservoir is present
            bool can_fill;
            if (is_plumbed_mode()) {
                can_fill = true;  // Plumbed mode: water line always available
            } else {
                can_fill = is_reservoir_present();  // Water tank mode: check reservoir
            }
            
            if (steam_level_low && !brewing && can_fill) {
                DEBUG_PRINT("Water: Steam boiler level low - starting fill cycle\n");
                
                // Step 1: Cut steam heater FIRST (CRITICAL!)
                control_outputs_t outputs;
                control_get_outputs(&outputs);
                g_steam_heater_was_on = (outputs.steam_heater > 0);
                
                if (g_steam_heater_was_on) {
                    // Turn off steam heater
                    control_set_output(1, 0, 1);  // Manual mode, 0%
                    DEBUG_PRINT("Water: Steam heater OFF (safety before fill)\n");
                }
                
                g_heater_off_time = now;
                g_fill_state = STEAM_FILL_HEATER_OFF;
            }
            break;
        }
            
        case STEAM_FILL_HEATER_OFF:
            // Wait brief delay after cutting heater before filling
            if (now - g_heater_off_time >= STEAM_HEATER_OFF_DELAY_MS) {
                // Step 2: Open fill solenoid (Path B)
                set_steam_fill_solenoid(true);
                
                // Step 3: Turn on pump
                control_set_pump(100);
                
                g_fill_start_time = now;
                g_fill_state = STEAM_FILL_ACTIVE;
                DEBUG_PRINT("Water: Steam fill active (pump ON, solenoid ON)\n");
            }
            break;
            
        case STEAM_FILL_ACTIVE:
            // Check timeout (safety)
            if (now - g_fill_start_time > STEAM_FILL_TIMEOUT_MS) {
                LOG_PRINT("Water: ERROR - Steam fill timeout! Stopping fill cycle\n");
                // Report error via protocol (alarm)
                protocol_send_alarm(ALARM_WATER_LOW, 1, 0);  // Severity 1 = error
                // Stop filling
                control_set_pump(0);
                set_steam_fill_solenoid(false);
                g_fill_state = STEAM_FILL_IDLE;
                // Reset monitoring state
                g_fill_level_check_time = 0;
                g_fill_level_last_state = false;
                // Restore heater if it was on
                if (g_steam_heater_was_on) {
                    // Heater will be restored by PID control
                    g_steam_heater_was_on = false;
                }
                return;
            }
            
            // Check if level probe is still reading correctly (sanity check)
            // If probe state hasn't changed after reasonable time, might be stuck
            // Uses module-scope variables for proper state tracking across calls
            if (now - g_fill_level_check_time > 5000) {  // Check every 5 seconds
                if (steam_level_low == g_fill_level_last_state && 
                    (now - g_fill_start_time) > 10000) {  // After 10 seconds of filling
                    // Level hasn't changed - probe might be stuck or water not flowing
                    DEBUG_PRINT("Water: WARNING - Steam level probe not changing after 10s fill\n");
                }
                g_fill_level_check_time = now;
                g_fill_level_last_state = steam_level_low;
            }
            
            // Check if level is now OK (probe touched)
            if (!steam_level_low) {
                // Step 4: Level OK, stop filling
                DEBUG_PRINT("Water: Steam boiler level OK - stopping fill\n");
                control_set_pump(0);
                set_steam_fill_solenoid(false);
                g_fill_state = STEAM_FILL_COMPLETE;
                // Reset monitoring state
                g_fill_level_check_time = 0;
                g_fill_level_last_state = false;
            }
            break;
            
        case STEAM_FILL_COMPLETE:
            // Brief delay before restoring heater
            if (now - g_fill_start_time > 1000) {  // 1s delay
                // Step 5: Restore steam heater (if it was on)
                if (g_steam_heater_was_on) {
                    // Return to auto mode (PID will control)
                    control_set_output(1, 0, 0);  // Auto mode
                    g_steam_heater_was_on = false;
                    DEBUG_PRINT("Water: Steam heater restored\n");
                }
                g_fill_state = STEAM_FILL_IDLE;
            }
            break;
    }
}

// =============================================================================
// Update Function (call periodically)
// =============================================================================

void water_management_update(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Update water LED (always, regardless of auto-fill)
    update_water_led();
    
    // Steam boiler auto-fill only applies to machines with steam level probe
    // Single boiler machines don't have a steam boiler to fill
    const machine_features_t* features = machine_get_features();
    if (!features || !features->has_steam_level_probe) {
        // No steam level probe - skip auto-fill (single boiler machines)
        return;
    }
    
    // Steam boiler auto-fill (only if enabled and not in safe state)
    if (!g_auto_fill_enabled || safety_is_safe_state()) {
        if (g_fill_state != STEAM_FILL_IDLE) {
            // Stop any active fill cycle
            set_steam_fill_solenoid(false);
            control_set_pump(0);
            g_fill_state = STEAM_FILL_IDLE;
        }
        return;
    }
    
    // Check fill status periodically
    if (now - g_last_fill_check < STEAM_FILL_CHECK_INTERVAL_MS) {
        return;
    }
    g_last_fill_check = now;
    
    // Update steam fill cycle
    update_steam_fill_cycle();
}

// =============================================================================
// Control Functions
// =============================================================================

void water_management_set_auto_fill(bool enabled) {
    g_auto_fill_enabled = enabled;
    
    if (!enabled && g_fill_state != STEAM_FILL_IDLE) {
        // Disable auto-fill, stop any active fill cycle
        set_steam_fill_solenoid(false);
        control_set_pump(0);
        g_fill_state = STEAM_FILL_IDLE;
    }
    
    DEBUG_PRINT("Water: Steam boiler auto-fill %s\n", enabled ? "enabled" : "disabled");
}

bool water_management_is_auto_fill_enabled(void) {
    return g_auto_fill_enabled;
}

bool water_management_is_filling(void) {
    return g_fill_state == STEAM_FILL_ACTIVE;
}

// =============================================================================
// Manual Control (for testing or override)
// =============================================================================

void water_management_set_fill_solenoid(bool on) {
    if (on && !is_reservoir_present()) {
        DEBUG_PRINT("Water: Cannot fill - reservoir not present\n");
        return;
    }
    
    if (on && is_brewing()) {
        DEBUG_PRINT("Water: Cannot fill - brew priority active\n");
        return;
    }
    
    set_steam_fill_solenoid(on);
    
    if (on) {
        // Also turn on pump for manual fill
        control_set_pump(100);
    } else {
        control_set_pump(0);
    }
}
