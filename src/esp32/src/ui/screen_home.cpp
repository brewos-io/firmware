/**
 * BrewOS Home Screen Implementation
 * 
 * Optimized for 480x480 round display
 * Compact layout to fit within safe area
 */

#include "platform/platform.h"
#include "ui/screen_home.h"
#include "display/theme.h"
#include "display/display_config.h"

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* brew_temp_label = nullptr;
static lv_obj_t* brew_temp_arc = nullptr;
static lv_obj_t* brew_setpoint_label = nullptr;
static lv_obj_t* brew_label_text = nullptr;  // "BREW" or "BOILER" label
static lv_obj_t* steam_temp_label = nullptr;
static lv_obj_t* steam_card = nullptr;       // Steam card container
static lv_obj_t* steam_title = nullptr;      // "STEAM" or "GROUP" label
static lv_obj_t* pressure_label = nullptr;
static lv_obj_t* status_label = nullptr;
static lv_obj_t* status_dot = nullptr;
static lv_obj_t* conn_icons = nullptr;
static lv_obj_t* wifi_icon = nullptr;
static lv_obj_t* scale_icon = nullptr;

// Cached machine type for dynamic updates
static uint8_t cached_machine_type = 0;

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_home_create(void) {
    LOG_I("Creating home screen...");
    
    // Create screen with dark background
    screen = lv_obj_create(NULL);
    // Ensure screen fills entire display
    lv_obj_set_size(screen, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Main temperature arc (centered) ===
    brew_temp_arc = lv_arc_create(screen);
    lv_obj_set_size(brew_temp_arc, 320, 320);
    lv_obj_center(brew_temp_arc);
    lv_arc_set_range(brew_temp_arc, 0, 100);
    lv_arc_set_value(brew_temp_arc, 0);
    lv_arc_set_bg_angles(brew_temp_arc, 135, 45);
    lv_arc_set_rotation(brew_temp_arc, 0);
    
    // Arc background
    lv_obj_set_style_arc_color(brew_temp_arc, COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_set_style_arc_width(brew_temp_arc, 10, LV_PART_MAIN);
    
    // Arc indicator  
    lv_obj_set_style_arc_color(brew_temp_arc, COLOR_ACCENT_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(brew_temp_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(brew_temp_arc, true, LV_PART_INDICATOR);
    
    // Hide knob
    lv_obj_set_style_bg_opa(brew_temp_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(brew_temp_arc, LV_OBJ_FLAG_CLICKABLE);
    
    // === Status at top (centered) ===
    lv_obj_t* status_row = lv_obj_create(screen);
    lv_obj_remove_style_all(status_row);
    lv_obj_set_size(status_row, 140, 24);
    lv_obj_align(status_row, LV_ALIGN_CENTER, 0, -115);
    lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    status_dot = lv_obj_create(status_row);
    lv_obj_set_size(status_dot, 8, 8);
    lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(status_dot, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_border_width(status_dot, 0, 0);
    
    status_label = lv_label_create(status_row);
    lv_label_set_text(status_label, "--");
    lv_obj_set_style_text_font(status_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(status_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_pad_left(status_label, 6, 0);
    
    // === Brew Temperature (large, centered) ===
    brew_temp_label = lv_label_create(screen);
    lv_label_set_text(brew_temp_label, "--°");
    lv_obj_set_style_text_font(brew_temp_label, FONT_TEMP, 0);
    lv_obj_set_style_text_color(brew_temp_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(brew_temp_label, LV_ALIGN_CENTER, 0, -35);
    
    // Brew label (dynamically updated based on machine type)
    brew_label_text = lv_label_create(screen);
    lv_label_set_text(brew_label_text, "BREW");
    lv_obj_set_style_text_font(brew_label_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(brew_label_text, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_letter_space(brew_label_text, 2, 0);
    lv_obj_align(brew_label_text, LV_ALIGN_CENTER, 0, 5);
    
    // Setpoint
    brew_setpoint_label = lv_label_create(screen);
    lv_label_set_text(brew_setpoint_label, "--");
    lv_obj_set_style_text_font(brew_setpoint_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(brew_setpoint_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(brew_setpoint_label, LV_ALIGN_CENTER, 0, 25);
    
    // === Bottom cards container ===
    lv_obj_t* cards_row = lv_obj_create(screen);
    lv_obj_remove_style_all(cards_row);
    lv_obj_set_size(cards_row, 220, 55);
    lv_obj_align(cards_row, LV_ALIGN_CENTER, 0, 85);
    lv_obj_set_flex_flow(cards_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cards_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // === Steam/Group Temperature card (adapts to machine type) ===
    steam_card = lv_obj_create(cards_row);
    lv_obj_set_size(steam_card, 100, 50);
    lv_obj_set_style_bg_color(steam_card, COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(steam_card, 10, 0);
    lv_obj_set_style_border_width(steam_card, 0, 0);
    lv_obj_set_style_pad_all(steam_card, 6, 0);
    lv_obj_clear_flag(steam_card, LV_OBJ_FLAG_SCROLLABLE);
    
    steam_title = lv_label_create(steam_card);
    lv_label_set_text(steam_title, "STEAM");
    lv_obj_set_style_text_font(steam_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(steam_title, COLOR_TEXT_MUTED, 0);
    lv_obj_align(steam_title, LV_ALIGN_TOP_LEFT, 0, -2);
    
    steam_temp_label = lv_label_create(steam_card);
    lv_label_set_text(steam_temp_label, "--°");
    lv_obj_set_style_text_font(steam_temp_label, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(steam_temp_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(steam_temp_label, LV_ALIGN_BOTTOM_LEFT, 0, 2);
    
    // === Pressure card ===
    lv_obj_t* pressure_card = lv_obj_create(cards_row);
    lv_obj_set_size(pressure_card, 100, 50);
    lv_obj_set_style_bg_color(pressure_card, COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(pressure_card, 10, 0);
    lv_obj_set_style_border_width(pressure_card, 0, 0);
    lv_obj_set_style_pad_all(pressure_card, 6, 0);
    lv_obj_clear_flag(pressure_card, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* pressure_title = lv_label_create(pressure_card);
    lv_label_set_text(pressure_title, "PRESSURE");
    lv_obj_set_style_text_font(pressure_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(pressure_title, COLOR_TEXT_MUTED, 0);
    lv_obj_align(pressure_title, LV_ALIGN_TOP_LEFT, 0, -2);
    
    pressure_label = lv_label_create(pressure_card);
    lv_label_set_text(pressure_label, "--");
    lv_obj_set_style_text_font(pressure_label, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(pressure_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(pressure_label, LV_ALIGN_BOTTOM_LEFT, 0, 2);
    
    lv_obj_t* bar_label = lv_label_create(pressure_card);
    lv_label_set_text(bar_label, "bar");
    lv_obj_set_style_text_font(bar_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(bar_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(bar_label, LV_ALIGN_BOTTOM_RIGHT, 0, 2);
    
    // === Connection icons (bottom) ===
    conn_icons = lv_obj_create(screen);
    lv_obj_remove_style_all(conn_icons);
    lv_obj_set_size(conn_icons, 60, 16);
    lv_obj_align(conn_icons, LV_ALIGN_CENTER, 0, 145);
    lv_obj_set_flex_flow(conn_icons, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(conn_icons, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // WiFi
    wifi_icon = lv_label_create(conn_icons);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, FONT_SMALL, 0);
    lv_obj_set_style_text_color(wifi_icon, COLOR_SUCCESS, 0);
    lv_obj_set_style_pad_right(wifi_icon, 10, 0);
    
    // Scale
    scale_icon = lv_label_create(conn_icons);
    lv_label_set_text(scale_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(scale_icon, FONT_SMALL, 0);
    lv_obj_set_style_text_color(scale_icon, COLOR_TEXT_MUTED, 0);
    
    LOG_I("Home screen created");
    return screen;
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_home_update(lv_obj_t* scr, const ui_state_t* state) {
    if (!state || !screen) return;
    
    // Update labels if machine type changed
    // machine_type: 0=unknown, 1=dual_boiler, 2=single_boiler, 3=heat_exchanger
    if (cached_machine_type != state->machine_type) {
        cached_machine_type = state->machine_type;
        
        // Update main temperature label based on machine type
        if (state->machine_type == 2) {  // Single boiler
            lv_label_set_text(brew_label_text, "BOILER");
        } else if (state->machine_type == 3) {  // Heat exchanger
            lv_label_set_text(brew_label_text, "GROUP");
        } else {  // Dual boiler or unknown
            lv_label_set_text(brew_label_text, "BREW");
        }
        
        // Update secondary card label
        if (state->machine_type == 2) {  // Single boiler - hide steam
            lv_obj_add_flag(steam_card, LV_OBJ_FLAG_HIDDEN);
        } else if (state->machine_type == 3) {  // Heat exchanger - show as BOILER
            lv_obj_clear_flag(steam_card, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(steam_title, "BOILER");
        } else {  // Dual boiler - show STEAM
            lv_obj_clear_flag(steam_card, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(steam_title, "STEAM");
        }
    }
    
    // Update main temperature display based on machine type
    char temp_str[16];
    float main_temp = state->brew_temp;
    float main_setpoint = state->brew_setpoint;
    
    // For HX machines, main display shows group temp
    if (state->machine_type == 3) {
        main_temp = state->group_temp;
        main_setpoint = 93.0f;  // Typical group target
    }
    
    snprintf(temp_str, sizeof(temp_str), "%.1f°", main_temp);
    lv_label_set_text(brew_temp_label, temp_str);
    
    // Update setpoint
    snprintf(temp_str, sizeof(temp_str), "> %.1f°C", main_setpoint);
    lv_label_set_text(brew_setpoint_label, temp_str);
    
    // Update brew arc (percentage of setpoint)
    if (main_setpoint > 0) {
        int pct = (int)((main_temp / main_setpoint) * 100);
        pct = LV_CLAMP(0, pct, 100);
        lv_arc_set_value(brew_temp_arc, pct);
        
        // Update arc color based on temperature state
        lv_color_t temp_color = theme_get_temp_color(main_temp, main_setpoint);
        lv_obj_set_style_arc_color(brew_temp_arc, temp_color, LV_PART_INDICATOR);
    }
    
    // Update secondary temperature card
    // For HX: shows steam boiler temp; For dual boiler: shows steam temp
    // For single boiler: card is hidden
    if (state->machine_type != 2) {  // Not single boiler
        snprintf(temp_str, sizeof(temp_str), "%.0f°", state->steam_temp);
        lv_label_set_text(steam_temp_label, temp_str);
    }
    
    // Update pressure
    snprintf(temp_str, sizeof(temp_str), "%.1f", state->pressure);
    lv_label_set_text(pressure_label, temp_str);
    
    // Pressure color
    lv_color_t pressure_color = theme_get_pressure_color(state->pressure);
    lv_obj_set_style_text_color(pressure_label, pressure_color, 0);
    
    // Update status
    const char* status_text = "READY";
    lv_color_t status_color = COLOR_SUCCESS;
    
    if (state->alarm_active) {
        status_text = "ALARM";
        status_color = COLOR_ERROR;
    } else if (state->water_low) {
        status_text = "LOW WATER";
        status_color = COLOR_WARNING;
    } else if (state->is_brewing) {
        status_text = "BREWING";
        status_color = COLOR_ACCENT_ORANGE;
    } else if (state->is_heating) {
        status_text = "HEATING";
        status_color = COLOR_WARNING;
    } else if (!state->pico_connected) {
        status_text = "OFFLINE";
        status_color = COLOR_ERROR;
    } else if (state->machine_state == UI_STATE_IDLE) {
        status_text = "OFF";
        status_color = COLOR_TEXT_MUTED;
    }
    
    lv_label_set_text(status_label, status_text);
    lv_obj_set_style_bg_color(status_dot, status_color, 0);
}
