/**
 * BrewOS Settings Screen Implementation
 * 
 * Round carousel-style menu for settings navigation
 * Supports inline temperature editing without leaving the menu
 * Optimized for 480x480 round display
 */

#include "platform/platform.h"
#include "ui/screen_settings.h"
#include "display/theme.h"
#include "display/display_config.h"

// Menu items with icons, names, and descriptions
static const char* item_icons[] = {
    LV_SYMBOL_SETTINGS,  // Brew Boiler Temp
    LV_SYMBOL_SETTINGS,  // Steam Boiler Temp
    LV_SYMBOL_DOWNLOAD,  // Brew by Weight
    LV_SYMBOL_BRIGHTNESS, // Brightness (sun/tint icon)
    LV_SYMBOL_POWER,     // Screen Timeout
    LV_SYMBOL_CLOUD,     // Cloud (upload icon)
    LV_SYMBOL_WIFI,      // WiFi
    LV_SYMBOL_LEFT       // Exit
};

static const char* item_names[] = {
    "Brew Boiler",
    "Steam Boiler",
    "Brew by Weight",
    "Brightness",
    "Screen Timeout",
    "Cloud",
    "WiFi Setup",
    "Exit"
};

static const char* item_descriptions[] = {
    "Set brew temperature",
    "Set steam temperature",
    "Enable weight-based brewing",
    "Adjust screen brightness",
    "Set screen idle time",
    "Pair with cloud",
    "Enter setup mode",
    "Return to home"
};

// Temperature limits
static const float BREW_MIN = 80.0f;
static const float BREW_MAX = 105.0f;
static const float STEAM_MIN = 120.0f;
static const float STEAM_MAX = 160.0f;
static const float TEMP_STEP = 0.5f;

// Display settings limits
static const uint8_t BRIGHTNESS_MIN = 10;
static const uint8_t BRIGHTNESS_MAX = 255;
static const uint8_t BRIGHTNESS_STEP = 5;
static const uint8_t TIMEOUT_MIN = 0;      // 0 = never
static const uint8_t TIMEOUT_MAX = 255;     // Max 255 seconds
static const uint8_t TIMEOUT_STEP = 5;

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* icon_label = nullptr;
static lv_obj_t* name_label = nullptr;
static lv_obj_t* value_label = nullptr;
static lv_obj_t* desc_label = nullptr;
static lv_obj_t* status_icons[SETTINGS_COUNT] = {nullptr};
static lv_obj_t* selector_arc = nullptr;

// State
static int selected_index = 0;
static bool editing_temp = false;  // True when editing a temperature value
static bool editing_display = false;  // True when editing brightness or timeout
static settings_select_callback_t select_callback = nullptr;
static float cached_brew_setpoint = 93.0f;
static float cached_steam_setpoint = 145.0f;
static float edit_temp_value = 0.0f;  // Value being edited
static uint8_t cached_brightness = 200;
static uint8_t cached_screen_timeout = 30;
static uint8_t edit_brightness_value = 200;  // Value being edited
static uint8_t edit_timeout_value = 30;  // Value being edited
static bool bbw_enabled = false;

// Callback for temperature changes
static void (*temp_change_callback)(bool is_steam, float temp) = nullptr;

// Callback for display settings changes
static settings_display_callback_t display_change_callback = nullptr;

// =============================================================================
// Helper Functions
// =============================================================================

static void update_value_display() {
    if (!value_label) return;
    
    char buf[24];
    
    switch (selected_index) {
        case SETTINGS_BREW_TEMP:
            if (editing_temp) {
                snprintf(buf, sizeof(buf), "%.1f°C", edit_temp_value);
                lv_obj_set_style_text_color(value_label, COLOR_SUCCESS, 0);
            } else {
                snprintf(buf, sizeof(buf), "%.0f°C", cached_brew_setpoint);
                lv_obj_set_style_text_color(value_label, COLOR_ACCENT_AMBER, 0);
            }
            lv_label_set_text(value_label, buf);
            lv_obj_clear_flag(value_label, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case SETTINGS_STEAM_TEMP:
            if (editing_temp) {
                snprintf(buf, sizeof(buf), "%.1f°C", edit_temp_value);
                lv_obj_set_style_text_color(value_label, COLOR_SUCCESS, 0);
            } else {
                snprintf(buf, sizeof(buf), "%.0f°C", cached_steam_setpoint);
                lv_obj_set_style_text_color(value_label, COLOR_ACCENT_ORANGE, 0);
            }
            lv_label_set_text(value_label, buf);
            lv_obj_clear_flag(value_label, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case SETTINGS_BREW_BY_WEIGHT:
            lv_label_set_text(value_label, bbw_enabled ? "ON" : "OFF");
            lv_obj_set_style_text_color(value_label, 
                bbw_enabled ? COLOR_SUCCESS : COLOR_TEXT_MUTED, 0);
            lv_obj_clear_flag(value_label, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case SETTINGS_BRIGHTNESS:
            if (editing_display) {
                snprintf(buf, sizeof(buf), "%d", edit_brightness_value);
                lv_obj_set_style_text_color(value_label, COLOR_SUCCESS, 0);
            } else {
                snprintf(buf, sizeof(buf), "%d", cached_brightness);
                lv_obj_set_style_text_color(value_label, COLOR_ACCENT_AMBER, 0);
            }
            lv_label_set_text(value_label, buf);
            lv_obj_clear_flag(value_label, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case SETTINGS_SCREEN_TIMEOUT:
            if (editing_display) {
                if (edit_timeout_value == 0) {
                    snprintf(buf, sizeof(buf), "Never");
                } else {
                    snprintf(buf, sizeof(buf), "%ds", edit_timeout_value);
                }
                lv_obj_set_style_text_color(value_label, COLOR_SUCCESS, 0);
            } else {
                if (cached_screen_timeout == 0) {
                    snprintf(buf, sizeof(buf), "Never");
                } else {
                    snprintf(buf, sizeof(buf), "%ds", cached_screen_timeout);
                }
                lv_obj_set_style_text_color(value_label, COLOR_ACCENT_AMBER, 0);
            }
            lv_label_set_text(value_label, buf);
            lv_obj_clear_flag(value_label, LV_OBJ_FLAG_HIDDEN);
            break;
            
        default:
            lv_obj_add_flag(value_label, LV_OBJ_FLAG_HIDDEN);
            break;
    }
    
    // Update description based on edit state
    if (desc_label) {
        if (editing_temp || editing_display) {
            lv_label_set_text(desc_label, "Rotate to adjust • Press to confirm");
            lv_obj_set_style_text_color(desc_label, COLOR_ACCENT_AMBER, 0);
        } else {
            lv_label_set_text(desc_label, item_descriptions[selected_index]);
            lv_obj_set_style_text_color(desc_label, COLOR_TEXT_MUTED, 0);
        }
    }
    
    // Update arc color based on edit state
    if (selector_arc) {
        if (editing_temp || editing_display) {
            lv_obj_set_style_arc_color(selector_arc, COLOR_SUCCESS, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_arc_color(selector_arc, COLOR_ACCENT_AMBER, LV_PART_INDICATOR);
        }
    }
}

static void update_menu_display() {
    // Update icon and name
    lv_label_set_text(icon_label, item_icons[selected_index]);
    lv_label_set_text(name_label, item_names[selected_index]);
    
    // Update arc position
    lv_arc_set_value(selector_arc, selected_index + 1);
    
    // Update dots
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        if (status_icons[i]) {
            if (i == selected_index) {
                lv_obj_set_style_bg_color(status_icons[i], COLOR_ACCENT_AMBER, 0);
            } else {
                lv_obj_set_style_bg_color(status_icons[i], COLOR_BG_ELEVATED, 0);
            }
        }
    }
    
    // Update value display
    update_value_display();
}

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_settings_create(void) {
    LOG_I("Creating settings screen...");
    
    // Create screen with dark background
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    
    // Create main container
    lv_obj_t* container = lv_obj_create(screen);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_center(container);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Title at top ===
    title_label = lv_label_create(container);
    lv_label_set_text(title_label, "Settings");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 50);
    
    // === Selection Arc (outer ring showing position) ===
    selector_arc = lv_arc_create(container);
    lv_obj_set_size(selector_arc, 420, 420);
    lv_obj_center(selector_arc);
    lv_arc_set_range(selector_arc, 0, SETTINGS_COUNT);
    lv_arc_set_value(selector_arc, 1);
    lv_arc_set_bg_angles(selector_arc, 0, 360);
    lv_arc_set_rotation(selector_arc, 270);
    
    // Arc background
    lv_obj_set_style_arc_color(selector_arc, COLOR_ARC_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(selector_arc, 4, LV_PART_MAIN);
    
    // Arc indicator
    lv_obj_set_style_arc_color(selector_arc, COLOR_ACCENT_AMBER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(selector_arc, 4, LV_PART_INDICATOR);
    
    // Hide knob
    lv_obj_set_style_bg_opa(selector_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(selector_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // === Large Icon (center) ===
    icon_label = lv_label_create(container);
    lv_label_set_text(icon_label, item_icons[selected_index]);
    lv_obj_set_style_text_font(icon_label, FONT_TEMP, 0);
    lv_obj_set_style_text_color(icon_label, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(icon_label, LV_ALIGN_CENTER, 0, -50);
    
    // === Item Name ===
    name_label = lv_label_create(container);
    lv_label_set_text(name_label, item_names[selected_index]);
    lv_obj_set_style_text_font(name_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(name_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(name_label, LV_ALIGN_CENTER, 0, 10);
    
    // === Current Value (for temps and BBW) ===
    value_label = lv_label_create(container);
    lv_label_set_text(value_label, "");
    lv_obj_set_style_text_font(value_label, FONT_LARGE, 0);  // Larger font for better visibility
    lv_obj_set_style_text_color(value_label, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, 40);
    
    // === Description ===
    desc_label = lv_label_create(container);
    lv_label_set_text(desc_label, item_descriptions[selected_index]);
    lv_obj_set_style_text_font(desc_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(desc_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(desc_label, LV_ALIGN_CENTER, 0, 75);
    
    // === Page dots at bottom ===
    lv_obj_t* dots_container = lv_obj_create(container);
    lv_obj_remove_style_all(dots_container);
    lv_obj_set_size(dots_container, SETTINGS_COUNT * 18, 12);
    lv_obj_align(dots_container, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_flex_flow(dots_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        status_icons[i] = lv_obj_create(dots_container);
        lv_obj_set_size(status_icons[i], 6, 6);
        lv_obj_set_style_radius(status_icons[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(status_icons[i], 0, 0);
        lv_obj_set_style_pad_left(status_icons[i], 3, 0);
        lv_obj_set_style_pad_right(status_icons[i], 3, 0);
        
        if (i == selected_index) {
            lv_obj_set_style_bg_color(status_icons[i], COLOR_ACCENT_AMBER, 0);
        } else {
            lv_obj_set_style_bg_color(status_icons[i], COLOR_BG_ELEVATED, 0);
        }
    }
    
    // === Hint ===
    lv_obj_t* hint = lv_label_create(container);
    lv_label_set_text(hint, "Rotate to browse • Press to select");
    lv_obj_set_style_text_font(hint, FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint, COLOR_TEXT_MUTED, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -80);
    
    // Update initial value display
    update_value_display();
    
    LOG_I("Settings screen created");
    return screen;
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_settings_update(const ui_state_t* state) {
    if (!screen || !state) return;
    
    // Only update cached values if not currently editing
    if (!editing_temp && !editing_display) {
        if (state->brew_setpoint > 0) {
            cached_brew_setpoint = state->brew_setpoint;
        }
        if (state->steam_setpoint > 0) {
            cached_steam_setpoint = state->steam_setpoint;
        }
        
        // Refresh display
        update_value_display();
    }
}

void screen_settings_navigate(int direction) {
    if (editing_temp) {
        // In edit mode: adjust temperature value
        float min_temp, max_temp;
        
        if (selected_index == SETTINGS_BREW_TEMP) {
            min_temp = BREW_MIN;
            max_temp = BREW_MAX;
        } else {
            min_temp = STEAM_MIN;
            max_temp = STEAM_MAX;
        }
        
        edit_temp_value += direction * TEMP_STEP;
        
        // Clamp to limits
        if (edit_temp_value < min_temp) edit_temp_value = min_temp;
        if (edit_temp_value > max_temp) edit_temp_value = max_temp;
        
        // Update display
        update_value_display();
        
        LOG_I("Editing temp: %.1f°C", edit_temp_value);
    } else if (editing_display) {
        // In edit mode: adjust brightness or timeout
        if (selected_index == SETTINGS_BRIGHTNESS) {
            int new_value = (int)edit_brightness_value + (direction * BRIGHTNESS_STEP);
            if (new_value < BRIGHTNESS_MIN) new_value = BRIGHTNESS_MIN;
            if (new_value > BRIGHTNESS_MAX) new_value = BRIGHTNESS_MAX;
            edit_brightness_value = (uint8_t)new_value;
            LOG_I("Editing brightness: %d", edit_brightness_value);
        } else if (selected_index == SETTINGS_SCREEN_TIMEOUT) {
            int new_value = (int)edit_timeout_value + (direction * TIMEOUT_STEP);
            if (new_value < TIMEOUT_MIN) new_value = TIMEOUT_MIN;
            if (new_value > TIMEOUT_MAX) new_value = TIMEOUT_MAX;
            edit_timeout_value = (uint8_t)new_value;
            LOG_I("Editing timeout: %ds", edit_timeout_value);
        }
        
        // Update display
        update_value_display();
    } else {
        // Normal menu navigation
        LOG_I("Settings navigate: direction=%d, current=%d", direction, selected_index);
        
        selected_index += direction;
        if (selected_index < 0) selected_index = SETTINGS_COUNT - 1;
        if (selected_index >= SETTINGS_COUNT) selected_index = 0;
        
        update_menu_display();
        
        LOG_I("Settings navigate: new index=%d", selected_index);
    }
}

settings_item_t screen_settings_get_selection(void) {
    return (settings_item_t)selected_index;
}

void screen_settings_select(void) {
    if (editing_temp) {
        // Confirm temperature edit
        bool is_steam = (selected_index == SETTINGS_STEAM_TEMP);
        
        // Update cached value
        if (is_steam) {
            cached_steam_setpoint = edit_temp_value;
        } else {
            cached_brew_setpoint = edit_temp_value;
        }
        
        // Call temperature change callback
        if (temp_change_callback) {
            temp_change_callback(is_steam, edit_temp_value);
        }
        
        LOG_I("Temperature confirmed: %s = %.1f°C", 
              is_steam ? "steam" : "brew", edit_temp_value);
        
        // Exit edit mode
        editing_temp = false;
        update_value_display();
        
        return;
    }
    
    if (editing_display) {
        // Confirm display settings edit
        if (selected_index == SETTINGS_BRIGHTNESS) {
            cached_brightness = edit_brightness_value;
            LOG_I("Brightness confirmed: %d", cached_brightness);
        } else if (selected_index == SETTINGS_SCREEN_TIMEOUT) {
            cached_screen_timeout = edit_timeout_value;
            LOG_I("Screen timeout confirmed: %ds", cached_screen_timeout);
        }
        
        // Call display change callback
        if (display_change_callback) {
            display_change_callback(cached_brightness, cached_screen_timeout);
        }
        
        // Exit edit mode
        editing_display = false;
        update_value_display();
        
        return;
    }
    
    // Handle selection based on item type
    switch (selected_index) {
        case SETTINGS_BREW_TEMP:
            // Enter edit mode for brew temperature
            editing_temp = true;
            edit_temp_value = cached_brew_setpoint;
            update_value_display();
            LOG_I("Editing brew temp, starting at %.1f°C", edit_temp_value);
            return;  // Don't call select callback
            
        case SETTINGS_STEAM_TEMP:
            // Enter edit mode for steam temperature
            editing_temp = true;
            edit_temp_value = cached_steam_setpoint;
            update_value_display();
            LOG_I("Editing steam temp, starting at %.1f°C", edit_temp_value);
            return;  // Don't call select callback
            
        case SETTINGS_BREW_BY_WEIGHT:
            // Toggle BBW inline
            bbw_enabled = !bbw_enabled;
            update_value_display();
            LOG_I("Brew by Weight: %s", bbw_enabled ? "ON" : "OFF");
            break;
            
        case SETTINGS_BRIGHTNESS:
            // Enter edit mode for brightness
            editing_display = true;
            edit_brightness_value = cached_brightness;
            update_value_display();
            LOG_I("Editing brightness, starting at %d", edit_brightness_value);
            return;  // Don't call select callback
            
        case SETTINGS_SCREEN_TIMEOUT:
            // Enter edit mode for screen timeout
            editing_display = true;
            edit_timeout_value = cached_screen_timeout;
            update_value_display();
            LOG_I("Editing screen timeout, starting at %ds", edit_timeout_value);
            return;  // Don't call select callback
            
        default:
            break;
    }
    
    // Call select callback for navigation items (Cloud, WiFi, Exit)
    if (select_callback) {
        select_callback((settings_item_t)selected_index);
    }
}

void screen_settings_set_select_callback(settings_select_callback_t callback) {
    select_callback = callback;
}

void screen_settings_set_temp_callback(void (*callback)(bool is_steam, float temp)) {
    temp_change_callback = callback;
}

void screen_settings_set_display_callback(settings_display_callback_t callback) {
    display_change_callback = callback;
}

void screen_settings_set_bbw_enabled(bool enabled) {
    bbw_enabled = enabled;
    update_value_display();
}

bool screen_settings_get_bbw_enabled(void) {
    return bbw_enabled;
}

void screen_settings_set_display_values(uint8_t brightness, uint8_t screenTimeout) {
    cached_brightness = brightness;
    cached_screen_timeout = screenTimeout;
    if (!editing_display) {
        update_value_display();
    }
}

bool screen_settings_is_editing(void) {
    return editing_temp || editing_display;
}

void screen_settings_cancel_edit(void) {
    if (editing_temp) {
        editing_temp = false;
        update_value_display();
        LOG_I("Edit cancelled");
    }
    if (editing_display) {
        editing_display = false;
        update_value_display();
        LOG_I("Display edit cancelled");
    }
}

void screen_settings_reset(void) {
    // Reset all editing states
    editing_temp = false;
    editing_display = false;
    selected_index = 0;
    
    // Update display to reflect reset state (only if screen is created)
    if (screen && icon_label && name_label) {
        update_menu_display();
    }
    
    LOG_I("Settings screen reset");
}

