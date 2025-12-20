/**
 * BrewOS Temperature Settings Screen Implementation
 * 
 * Interactive temperature adjustment using rotary encoder
 */

#include "platform/platform.h"
#include "ui/screen_temp.h"
#include "display/theme.h"
#include "display/display_config.h"

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* brew_card = nullptr;
static lv_obj_t* brew_temp_label = nullptr;
static lv_obj_t* brew_sp_label = nullptr;
static lv_obj_t* steam_card = nullptr;
static lv_obj_t* steam_temp_label = nullptr;
static lv_obj_t* steam_sp_label = nullptr;
static lv_obj_t* hint_label = nullptr;

// State
static temp_edit_mode_t current_mode = TEMP_EDIT_NONE;
static float brew_setpoint = 93.0f;
static float steam_setpoint = 145.0f;
static temp_change_callback_t change_callback = nullptr;
static uint8_t cached_machine_type = 0;

// Limits
static const float BREW_MIN = 80.0f;
static const float BREW_MAX = 105.0f;
static const float STEAM_MIN = 120.0f;
static const float STEAM_MAX = 160.0f;
static const float TEMP_STEP = 0.5f;

// Labels for title elements (to update dynamically)
static lv_obj_t* brew_title_label = nullptr;
static lv_obj_t* steam_title_label = nullptr;

// =============================================================================
// Helper Functions
// =============================================================================

static void update_card_style(lv_obj_t* card, bool selected, bool editing) {
    if (editing) {
        lv_obj_set_style_border_color(card, COLOR_ACCENT_PRIMARY, 0);
        lv_obj_set_style_border_width(card, 3, 0);
        lv_obj_set_style_bg_color(card, lv_color_darken(COLOR_ACCENT_PRIMARY, LV_OPA_80), 0);
    } else if (selected) {
        lv_obj_set_style_border_color(card, COLOR_ACCENT_AMBER, 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lv_obj_set_style_bg_color(card, COLOR_BG_CARD, 0);
    } else {
        lv_obj_set_style_border_color(card, COLOR_BG_ELEVATED, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_bg_color(card, COLOR_BG_CARD, 0);
    }
}

static void update_display() {
    char buf[16];
    
    // Brew setpoint
    snprintf(buf, sizeof(buf), "%.1f°C", brew_setpoint);
    lv_label_set_text(brew_sp_label, buf);
    
    // Steam setpoint
    snprintf(buf, sizeof(buf), "%.1f°C", steam_setpoint);
    lv_label_set_text(steam_sp_label, buf);
    
    // Update card styles
    bool brew_selected = (current_mode == TEMP_EDIT_NONE);  // Default to brew selected
    bool steam_selected = false;
    
    if (current_mode == TEMP_EDIT_BREW) {
        update_card_style(brew_card, true, true);
        update_card_style(steam_card, false, false);
    } else if (current_mode == TEMP_EDIT_STEAM) {
        update_card_style(brew_card, false, false);
        update_card_style(steam_card, true, true);
    } else {
        // Selection mode - alternate based on focus
        static bool brew_focused = true;
        update_card_style(brew_card, brew_focused, false);
        update_card_style(steam_card, !brew_focused, false);
    }
    
    // Update hint
    if (current_mode == TEMP_EDIT_NONE) {
        lv_label_set_text(hint_label, "Rotate to select • Press to edit\nLong press to exit");
    } else if (current_mode == TEMP_EDIT_BREW) {
        lv_label_set_text(hint_label, "Editing BREW • Rotate to adjust\nPress to confirm");
    } else {
        lv_label_set_text(hint_label, "Editing STEAM • Rotate to adjust\nPress to confirm");
    }
}

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_temp_create(void) {
    LOG_I("Creating temperature settings screen...");
    
    // Create screen
    screen = lv_obj_create(NULL);
    // Ensure screen fills entire display
    lv_obj_set_size(screen, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Temperature");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 60);
    
    // Brew temperature card
    brew_card = lv_obj_create(screen);
    lv_obj_set_size(brew_card, 160, 100);
    lv_obj_align(brew_card, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(brew_card, COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(brew_card, 16, 0);
    lv_obj_set_style_border_color(brew_card, COLOR_BG_ELEVATED, 0);
    lv_obj_set_style_border_width(brew_card, 1, 0);
    lv_obj_set_style_pad_all(brew_card, 12, 0);
    lv_obj_clear_flag(brew_card, LV_OBJ_FLAG_SCROLLABLE);
    
    brew_title_label = lv_label_create(brew_card);
    lv_label_set_text(brew_title_label, "BREW");
    lv_obj_set_style_text_font(brew_title_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(brew_title_label, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_letter_space(brew_title_label, 2, 0);
    lv_obj_align(brew_title_label, LV_ALIGN_TOP_MID, 0, 0);
    
    brew_sp_label = lv_label_create(brew_card);
    lv_label_set_text(brew_sp_label, "93.0°C");
    lv_obj_set_style_text_font(brew_sp_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(brew_sp_label, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(brew_sp_label, LV_ALIGN_CENTER, 0, 8);
    
    brew_temp_label = lv_label_create(brew_card);
    lv_label_set_text(brew_temp_label, "Current: --.-°C");
    lv_obj_set_style_text_font(brew_temp_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(brew_temp_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(brew_temp_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Steam temperature card
    steam_card = lv_obj_create(screen);
    lv_obj_set_size(steam_card, 160, 100);
    lv_obj_align(steam_card, LV_ALIGN_CENTER, 0, 90);
    lv_obj_set_style_bg_color(steam_card, COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(steam_card, 16, 0);
    lv_obj_set_style_border_color(steam_card, COLOR_BG_ELEVATED, 0);
    lv_obj_set_style_border_width(steam_card, 1, 0);
    lv_obj_set_style_pad_all(steam_card, 12, 0);
    lv_obj_clear_flag(steam_card, LV_OBJ_FLAG_SCROLLABLE);
    
    steam_title_label = lv_label_create(steam_card);
    lv_label_set_text(steam_title_label, "STEAM");
    lv_obj_set_style_text_font(steam_title_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(steam_title_label, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_letter_space(steam_title_label, 2, 0);
    lv_obj_align(steam_title_label, LV_ALIGN_TOP_MID, 0, 0);
    
    steam_sp_label = lv_label_create(steam_card);
    lv_label_set_text(steam_sp_label, "145.0°C");
    lv_obj_set_style_text_font(steam_sp_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(steam_sp_label, COLOR_ACCENT_ORANGE, 0);
    lv_obj_align(steam_sp_label, LV_ALIGN_CENTER, 0, 8);
    
    steam_temp_label = lv_label_create(steam_card);
    lv_label_set_text(steam_temp_label, "Current: --.-°C");
    lv_obj_set_style_text_font(steam_temp_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(steam_temp_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(steam_temp_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Hint label
    hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, "Rotate to select • Press to edit\nLong press to exit");
    lv_obj_set_style_text_font(hint_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint_label, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -60);
    
    // Initialize state
    current_mode = TEMP_EDIT_NONE;
    update_display();
    
    LOG_I("Temperature settings screen created");
    return screen;
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_temp_update(const ui_state_t* state) {
    if (!state || !screen) return;
    
    // Update labels if machine type changed
    // machine_type: 0=unknown, 1=dual_boiler, 2=single_boiler, 3=heat_exchanger
    if (cached_machine_type != state->machine_type) {
        cached_machine_type = state->machine_type;
        
        if (state->machine_type == 2) {  // Single boiler
            lv_label_set_text(brew_title_label, "BOILER");
            // Hide steam card for single boiler
            lv_obj_add_flag(steam_card, LV_OBJ_FLAG_HIDDEN);
            // Reposition brew card to center
            lv_obj_align(brew_card, LV_ALIGN_CENTER, 0, 30);
        } else if (state->machine_type == 3) {  // Heat exchanger
            // HX: show only steam boiler (controls HX)
            lv_obj_add_flag(brew_card, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(steam_title_label, "BOILER");
            // Reposition steam card to center
            lv_obj_align(steam_card, LV_ALIGN_CENTER, 0, 30);
        } else {  // Dual boiler or unknown - show both
            lv_label_set_text(brew_title_label, "BREW");
            lv_label_set_text(steam_title_label, "STEAM");
            lv_obj_clear_flag(brew_card, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(steam_card, LV_OBJ_FLAG_HIDDEN);
            // Reset positions
            lv_obj_align(brew_card, LV_ALIGN_CENTER, 0, -30);
            lv_obj_align(steam_card, LV_ALIGN_CENTER, 0, 90);
        }
    }
    
    // Update current temperature displays
    char buf[32];
    
    // For HX: brew temp comes from group_temp
    float display_brew_temp = (state->machine_type == 3) ? state->group_temp : state->brew_temp;
    snprintf(buf, sizeof(buf), "Current: %.1f°C", display_brew_temp);
    lv_label_set_text(brew_temp_label, buf);
    
    snprintf(buf, sizeof(buf), "Current: %.1f°C", state->steam_temp);
    lv_label_set_text(steam_temp_label, buf);
    
    // Sync setpoints from state (if not editing)
    if (current_mode == TEMP_EDIT_NONE) {
        if (state->brew_setpoint > 0) {
            brew_setpoint = state->brew_setpoint;
        }
        if (state->steam_setpoint > 0) {
            steam_setpoint = state->steam_setpoint;
        }
        update_display();
    }
}

// =============================================================================
// Encoder Handling
// =============================================================================

// Track which card is focused when not editing
static bool brew_focused = true;

void screen_temp_encoder(int direction) {
    if (current_mode == TEMP_EDIT_NONE) {
        // Selection mode - toggle between brew and steam
        if (direction != 0) {
            brew_focused = !brew_focused;
            update_card_style(brew_card, brew_focused, false);
            update_card_style(steam_card, !brew_focused, false);
        }
    } else if (current_mode == TEMP_EDIT_BREW) {
        // Adjust brew temperature
        float new_temp = brew_setpoint + (direction * TEMP_STEP);
        new_temp = LV_CLAMP(BREW_MIN, new_temp, BREW_MAX);
        if (new_temp != brew_setpoint) {
            brew_setpoint = new_temp;
            update_display();
        }
    } else if (current_mode == TEMP_EDIT_STEAM) {
        // Adjust steam temperature
        float new_temp = steam_setpoint + (direction * TEMP_STEP);
        new_temp = LV_CLAMP(STEAM_MIN, new_temp, STEAM_MAX);
        if (new_temp != steam_setpoint) {
            steam_setpoint = new_temp;
            update_display();
        }
    }
}

bool screen_temp_select(void) {
    if (current_mode == TEMP_EDIT_NONE) {
        // Enter edit mode for focused card
        current_mode = brew_focused ? TEMP_EDIT_BREW : TEMP_EDIT_STEAM;
        update_display();
        LOG_I("Temp edit mode: %s", current_mode == TEMP_EDIT_BREW ? "brew" : "steam");
        return true;
    } else {
        // Confirm edit and send callback
        bool is_steam = (current_mode == TEMP_EDIT_STEAM);
        float temp = is_steam ? steam_setpoint : brew_setpoint;
        
        if (change_callback) {
            change_callback(is_steam, temp);
        }
        
        LOG_I("Temp set: %s = %.1f°C", is_steam ? "steam" : "brew", temp);
        
        // Return to selection mode
        current_mode = TEMP_EDIT_NONE;
        update_display();
        return true;
    }
}

temp_edit_mode_t screen_temp_get_mode(void) {
    return current_mode;
}

void screen_temp_set_callback(temp_change_callback_t callback) {
    change_callback = callback;
}

