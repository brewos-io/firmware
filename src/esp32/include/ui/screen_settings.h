/**
 * BrewOS Settings Screen
 * 
 * Round carousel-style settings menu with inline editing:
 * - Brew Boiler Temp (edit directly with encoder)
 * - Steam Boiler Temp (edit directly with encoder)
 * - Brew by Weight (on/off toggle)
 * - Cloud (pairing)
 * - WiFi (enter setup mode)
 * - Exit
 */

#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

#include <lvgl.h>
#include "ui.h"

/**
 * Settings menu items
 */
typedef enum {
    SETTINGS_BREW_TEMP,
    SETTINGS_STEAM_TEMP,
    SETTINGS_BREW_BY_WEIGHT,
    SETTINGS_BRIGHTNESS,
    SETTINGS_SCREEN_TIMEOUT,
    SETTINGS_CLOUD,
    SETTINGS_WIFI,
    SETTINGS_EXIT,
    SETTINGS_COUNT
} settings_item_t;

/**
 * Create the settings screen
 */
lv_obj_t* screen_settings_create(void);

/**
 * Update settings screen with current state
 */
void screen_settings_update(const ui_state_t* state);

/**
 * Navigate menu or adjust value (encoder rotation)
 * In edit mode: adjusts temperature
 * In browse mode: navigates menu items
 */
void screen_settings_navigate(int direction);

/**
 * Get current selection
 */
settings_item_t screen_settings_get_selection(void);

/**
 * Select current item or confirm edit (encoder press)
 * For temps: enters edit mode, second press confirms
 * For BBW: toggles on/off
 * For others: triggers callback
 */
void screen_settings_select(void);

/**
 * Callback for menu item selection (Cloud, WiFi, Exit)
 */
typedef void (*settings_select_callback_t)(settings_item_t item);
void screen_settings_set_select_callback(settings_select_callback_t callback);

/**
 * Callback for temperature changes
 * Called when user confirms a temperature edit
 */
void screen_settings_set_temp_callback(void (*callback)(bool is_steam, float temp));

/**
 * Callback for display settings changes
 * Called when user confirms brightness or screen timeout edit
 */
typedef void (*settings_display_callback_t)(uint8_t brightness, uint8_t screenTimeout);
void screen_settings_set_display_callback(settings_display_callback_t callback);

/**
 * Set Brew by Weight enabled state
 */
void screen_settings_set_bbw_enabled(bool enabled);

/**
 * Get Brew by Weight enabled state
 */
bool screen_settings_get_bbw_enabled(void);

/**
 * Set initial display settings values
 */
void screen_settings_set_display_values(uint8_t brightness, uint8_t screenTimeout);

/**
 * Check if currently editing a temperature value
 */
bool screen_settings_is_editing(void);

/**
 * Cancel current edit (if editing)
 * Used for long-press to cancel
 */
void screen_settings_cancel_edit(void);

/**
 * Reset settings screen state (call when entering the screen)
 * Clears any editing state from previous visits
 */
void screen_settings_reset(void);

#endif // SCREEN_SETTINGS_H

