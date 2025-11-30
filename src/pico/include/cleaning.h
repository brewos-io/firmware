/**
 * ECM Pico Firmware - Cleaning Mode
 * 
 * Implements cleaning mode functionality similar to ECM Synchronika:
 * - Brew counter (tracks cycles >15 seconds)
 * - Cleaning reminder after threshold (default 100 cycles)
 * - Cleaning cycle (backflush with blind filter)
 * 
 * Cleaning Process:
 * 1. User inserts blind filter with cleaning tablet
 * 2. User activates cleaning mode (via ESP32 or brew lever)
 * 3. Pump runs for ~10 seconds (backflushing)
 * 4. User can stop manually or let it auto-stop
 * 5. Repeat multiple times as needed
 * 6. Rinse with clean water
 */

#ifndef CLEANING_H
#define CLEANING_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Configuration
// =============================================================================

#define CLEANING_DEFAULT_THRESHOLD    100     // Default reminder after 100 brews
#define CLEANING_MIN_THRESHOLD        10      // Minimum threshold
#define CLEANING_MAX_THRESHOLD        1000    // Maximum threshold
#define CLEANING_CYCLE_MIN_TIME_MS    15000   // Minimum brew time to count as cycle (15s)
#define CLEANING_CYCLE_DURATION_MS    10000   // Cleaning cycle pump duration (10s)

// Flash wear reduction: save every N brews instead of every brew
#define CLEANING_FLASH_SAVE_INTERVAL  10      // Save to flash every 10 brews

// =============================================================================
// Cleaning State
// =============================================================================

typedef enum {
    CLEANING_IDLE = 0,           // Not in cleaning mode
    CLEANING_ACTIVE,             // Cleaning cycle active (pump running)
    CLEANING_PAUSED              // Cleaning cycle paused (between cycles)
} cleaning_state_t;

// =============================================================================
// Functions
// =============================================================================

/**
 * Initialize cleaning mode system
 */
void cleaning_init(void);

/**
 * Update cleaning mode (call in main loop)
 */
void cleaning_update(void);

/**
 * Start cleaning cycle
 * 
 * IMPORTANT: Cleaning should only be performed when the machine is at operating
 * temperature (STATE_READY). This ensures effective cleaning with hot water.
 * 
 * @return true if started successfully, false if machine is not ready or other conditions not met
 */
bool cleaning_start_cycle(void);

/**
 * Stop cleaning cycle
 */
void cleaning_stop_cycle(void);

/**
 * Check if cleaning mode is active
 */
bool cleaning_is_active(void);

/**
 * Get current cleaning state
 */
cleaning_state_t cleaning_get_state(void);

/**
 * Get brew counter (number of brewing cycles)
 */
uint16_t cleaning_get_brew_count(void);

/**
 * Reset brew counter (after cleaning completed)
 */
void cleaning_reset_brew_count(void);

/**
 * Get cleaning reminder threshold
 */
uint16_t cleaning_get_threshold(void);

/**
 * Set cleaning reminder threshold (10-200 cycles)
 * @param threshold Number of cycles before reminder
 * @return true if threshold is valid and set
 */
bool cleaning_set_threshold(uint16_t threshold);

/**
 * Check if cleaning reminder should be shown
 * @return true if brew count >= threshold
 */
bool cleaning_is_reminder_due(void);

/**
 * Record a brewing cycle (called when brew completes)
 * Only counts if brew duration >= 15 seconds
 * @param brew_duration_ms Duration of the brew cycle in milliseconds
 */
void cleaning_record_brew_cycle(uint32_t brew_duration_ms);

/**
 * Force save cleaning state to flash
 * Call this on shutdown or periodically to ensure data is persisted.
 * Flash wear reduction: normally saves happen every CLEANING_FLASH_SAVE_INTERVAL brews.
 * @return true if saved successfully or no pending changes
 */
bool cleaning_force_save(void);

/**
 * Check if there are unsaved changes
 * @return true if there are pending changes not yet saved to flash
 */
bool cleaning_has_unsaved_changes(void);

#endif // CLEANING_H

