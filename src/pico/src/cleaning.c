/**
 * ECM Pico Firmware - Cleaning Mode Implementation
 * 
 * Implements cleaning mode with brew counter and cleaning cycles.
 */

#include "cleaning.h"
#include "control.h"
#include "state.h"
#include "safety.h"
#include "pcb_config.h"
#include "hardware.h"
#include "config_persistence.h"
#include "config.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdint.h>  // For UINT16_MAX

// =============================================================================
// Private State
// =============================================================================

static cleaning_state_t g_cleaning_state = CLEANING_IDLE;
static uint16_t g_brew_count = 0;
static uint16_t g_cleaning_threshold = CLEANING_DEFAULT_THRESHOLD;
static uint32_t g_cleaning_cycle_start = 0;
static bool g_cleaning_initialized = false;

// Flash wear reduction: track pending saves and last saved value
static uint16_t g_last_saved_brew_count = 0;  // Last brew count written to flash
static bool g_dirty = false;                   // True if we have unsaved changes

// =============================================================================
// Initialization
// =============================================================================

void cleaning_init(void) {
    if (g_cleaning_initialized) {
        return;
    }
    
    g_cleaning_state = CLEANING_IDLE;
    g_cleaning_cycle_start = 0;
    
    // Load brew count and threshold from config persistence
    config_persistence_get_cleaning(&g_brew_count, &g_cleaning_threshold);
    
    // Track last saved value for flash wear reduction
    g_last_saved_brew_count = g_brew_count;
    g_dirty = false;
    
    // Validate threshold (in case of corrupted data)
    if (g_cleaning_threshold < CLEANING_MIN_THRESHOLD || 
        g_cleaning_threshold > CLEANING_MAX_THRESHOLD) {
        g_cleaning_threshold = CLEANING_DEFAULT_THRESHOLD;
        // Save corrected threshold immediately (configuration fix)
        config_persistence_save_cleaning(g_brew_count, g_cleaning_threshold);
        g_last_saved_brew_count = g_brew_count;
    }
    
    g_cleaning_initialized = true;
    
    DEBUG_PRINT("Cleaning mode initialized (brew_count=%d, threshold=%d)\n", 
               g_brew_count, g_cleaning_threshold);
}

// =============================================================================
// Brew Counter
// =============================================================================

void cleaning_record_brew_cycle(uint32_t brew_duration_ms) {
    // Only count brews that last at least 15 seconds
    // This matches ECM Synchronika behavior
    if (brew_duration_ms >= CLEANING_CYCLE_MIN_TIME_MS) {
        // Saturate at UINT16_MAX to prevent overflow (losing brew count)
        if (g_brew_count < UINT16_MAX) {
            g_brew_count++;
        }
        g_dirty = true;
        
        DEBUG_PRINT("Cleaning: Brew cycle recorded (count=%d, duration=%lu ms)\n", 
                    g_brew_count, brew_duration_ms);
        
        // Flash wear reduction: only save to flash every N brews
        // This reduces flash writes by 90% (from every brew to every 10 brews)
        uint16_t brews_since_save = g_brew_count - g_last_saved_brew_count;
        if (brews_since_save >= CLEANING_FLASH_SAVE_INTERVAL) {
            config_persistence_save_cleaning(g_brew_count, g_cleaning_threshold);
            g_last_saved_brew_count = g_brew_count;
            g_dirty = false;
            DEBUG_PRINT("Cleaning: Saved to flash (interval save)\n");
        }
        
        // Also save when crossing the threshold (important milestone)
        if (g_brew_count == g_cleaning_threshold) {
            config_persistence_save_cleaning(g_brew_count, g_cleaning_threshold);
            g_last_saved_brew_count = g_brew_count;
            g_dirty = false;
            DEBUG_PRINT("Cleaning: Saved to flash (threshold reached)\n");
        }
    }
}

uint16_t cleaning_get_brew_count(void) {
    return g_brew_count;
}

void cleaning_reset_brew_count(void) {
    g_brew_count = 0;
    DEBUG_PRINT("Cleaning: Brew counter reset\n");
    
    // Save immediately - this is an explicit user action
    config_persistence_save_cleaning(g_brew_count, g_cleaning_threshold);
    g_last_saved_brew_count = g_brew_count;
    g_dirty = false;
}

uint16_t cleaning_get_threshold(void) {
    return g_cleaning_threshold;
}

bool cleaning_set_threshold(uint16_t threshold) {
    if (threshold < CLEANING_MIN_THRESHOLD || threshold > CLEANING_MAX_THRESHOLD) {
        return false;
    }
    
    g_cleaning_threshold = threshold;
    DEBUG_PRINT("Cleaning: Threshold set to %d cycles\n", threshold);
    
    // Save immediately - this is an explicit user action
    config_persistence_save_cleaning(g_brew_count, g_cleaning_threshold);
    g_last_saved_brew_count = g_brew_count;
    g_dirty = false;
    
    return true;
}

bool cleaning_is_reminder_due(void) {
    return (g_brew_count >= g_cleaning_threshold);
}

// =============================================================================
// Cleaning Cycle
// =============================================================================

bool cleaning_start_cycle(void) {
    // Can't start if already active or in safe state
    if (g_cleaning_state != CLEANING_IDLE) {
        return false;
    }
    
    if (state_is_brewing()) {
        return false;  // Can't start cleaning while brewing
    }
    
    if (safety_is_safe_state()) {
        return false;  // Can't start cleaning in safe state
    }
    
    // IMPORTANT: Cleaning should only be performed when machine is at operating temperature
    // This ensures effective cleaning with hot water and proper machine state
    if (!state_is_ready()) {
        LOG_PRINT("Cleaning: Cannot start - machine not at operating temperature (must be STATE_READY)\n");
        return false;  // Machine must be at operating temperature (STATE_READY)
    }
    
    g_cleaning_state = CLEANING_ACTIVE;
    g_cleaning_cycle_start = to_ms_since_boot(get_absolute_time());
    
    // Start pump and open brew solenoid (backflush)
    control_set_pump(100);  // 100% pump power
    
    const pcb_config_t* pcb = pcb_config_get();
    if (pcb && pcb->pins.relay_brew_solenoid >= 0) {
        hw_set_gpio(pcb->pins.relay_brew_solenoid, true);  // Open solenoid
    }
    
    LOG_PRINT("Cleaning: Cycle started (brew_count=%d)\n", g_brew_count);
    
    return true;
}

void cleaning_stop_cycle(void) {
    if (g_cleaning_state == CLEANING_IDLE) {
        return;
    }
    
    // Stop pump and close solenoid
    control_set_pump(0);
    
    const pcb_config_t* pcb = pcb_config_get();
    if (pcb && pcb->pins.relay_brew_solenoid >= 0) {
        hw_set_gpio(pcb->pins.relay_brew_solenoid, false);  // Close solenoid
    }
    
    g_cleaning_state = CLEANING_IDLE;
    g_cleaning_cycle_start = 0;
    
    LOG_PRINT("Cleaning: Cycle stopped\n");
}

void cleaning_update(void) {
    if (g_cleaning_state == CLEANING_ACTIVE) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        uint32_t elapsed = now - g_cleaning_cycle_start;
        
        // Auto-stop after CLEANING_CYCLE_DURATION_MS (10 seconds)
        if (elapsed >= CLEANING_CYCLE_DURATION_MS) {
            cleaning_stop_cycle();
            DEBUG_PRINT("Cleaning: Cycle auto-stopped after %lu ms\n", elapsed);
        }
        
        // Also stop if brew lever is released (user manual stop)
        // This is handled in state.c by checking brew switch
    }
}

bool cleaning_is_active(void) {
    return (g_cleaning_state != CLEANING_IDLE);
}

cleaning_state_t cleaning_get_state(void) {
    return g_cleaning_state;
}

// =============================================================================
// Flash Wear Reduction Functions
// =============================================================================

bool cleaning_force_save(void) {
    if (!g_dirty) {
        return true;  // No pending changes
    }
    
    if (config_persistence_save_cleaning(g_brew_count, g_cleaning_threshold)) {
        g_last_saved_brew_count = g_brew_count;
        g_dirty = false;
        DEBUG_PRINT("Cleaning: Force saved to flash\n");
        return true;
    }
    
    DEBUG_PRINT("Cleaning: Force save failed!\n");
    return false;
}

bool cleaning_has_unsaved_changes(void) {
    return g_dirty;
}

