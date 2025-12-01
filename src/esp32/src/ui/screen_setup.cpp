/**
 * BrewOS Setup Screen Implementation
 * 
 * WiFi AP setup screen for first-time configuration
 * Optimized for 480x480 round display
 */

#include "platform/platform.h"
#include "ui/screen_setup.h"
#include "display/theme.h"
#include "display/display_config.h"

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* ssid_label = nullptr;
static lv_obj_t* password_label = nullptr;
static lv_obj_t* ip_label = nullptr;
static lv_obj_t* status_label = nullptr;
static lv_obj_t* spinner = nullptr;

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_setup_create(void) {
    LOG_I("Creating setup screen...");
    
    // Create screen with dark background
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === WiFi Icon at top ===
    lv_obj_t* wifi_icon = lv_label_create(screen);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, FONT_XLARGE, 0);
    lv_obj_set_style_text_color(wifi_icon, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_CENTER, 0, -140);
    
    // === Title ===
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "WiFi Setup");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -90);
    
    // === Compact Card for credentials ===
    lv_obj_t* card = lv_obj_create(screen);
    lv_obj_set_size(card, 280, 130);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(card, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, RADIUS_NORMAL, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // SSID row
    lv_obj_t* ssid_row = lv_obj_create(card);
    lv_obj_remove_style_all(ssid_row);
    lv_obj_set_size(ssid_row, lv_pct(100), 32);
    lv_obj_align(ssid_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(ssid_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ssid_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* ssid_key = lv_label_create(ssid_row);
    lv_label_set_text(ssid_key, "Network");
    lv_obj_set_style_text_font(ssid_key, FONT_SMALL, 0);
    lv_obj_set_style_text_color(ssid_key, COLOR_TEXT_MUTED, 0);
    
    ssid_label = lv_label_create(ssid_row);
    lv_label_set_text(ssid_label, "BrewOS-XXXX");
    lv_obj_set_style_text_font(ssid_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(ssid_label, COLOR_ACCENT_AMBER, 0);
    
    // Password row
    lv_obj_t* pass_row = lv_obj_create(card);
    lv_obj_remove_style_all(pass_row);
    lv_obj_set_size(pass_row, lv_pct(100), 32);
    lv_obj_align(pass_row, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_flex_flow(pass_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pass_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* pass_key = lv_label_create(pass_row);
    lv_label_set_text(pass_key, "Password");
    lv_obj_set_style_text_font(pass_key, FONT_SMALL, 0);
    lv_obj_set_style_text_color(pass_key, COLOR_TEXT_MUTED, 0);
    
    password_label = lv_label_create(pass_row);
    lv_label_set_text(password_label, "brewos123");
    lv_obj_set_style_text_font(password_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(password_label, COLOR_TEXT_PRIMARY, 0);
    
    // IP row
    lv_obj_t* ip_row = lv_obj_create(card);
    lv_obj_remove_style_all(ip_row);
    lv_obj_set_size(ip_row, lv_pct(100), 32);
    lv_obj_align(ip_row, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_set_flex_flow(ip_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ip_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* ip_key = lv_label_create(ip_row);
    lv_label_set_text(ip_key, "Open");
    lv_obj_set_style_text_font(ip_key, FONT_SMALL, 0);
    lv_obj_set_style_text_color(ip_key, COLOR_TEXT_MUTED, 0);
    
    ip_label = lv_label_create(ip_row);
    lv_label_set_text(ip_label, "192.168.4.1");
    lv_obj_set_style_text_font(ip_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(ip_label, COLOR_INFO, 0);
    
    // === Status at bottom ===
    lv_obj_t* status_container = lv_obj_create(screen);
    lv_obj_remove_style_all(status_container);
    lv_obj_set_size(status_container, 240, 30);
    lv_obj_align(status_container, LV_ALIGN_CENTER, 0, 120);
    lv_obj_set_flex_flow(status_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Spinner
    spinner = lv_spinner_create(status_container, 1000, 60);
    lv_obj_set_size(spinner, 20, 20);
    lv_obj_set_style_arc_color(spinner, COLOR_ACCENT_AMBER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 3, 0);
    
    status_label = lv_label_create(status_container);
    lv_label_set_text(status_label, "Waiting...");
    lv_obj_set_style_text_font(status_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(status_label, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_pad_left(status_label, 8, 0);
    
    LOG_I("Setup screen created");
    return screen;
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_setup_update(const ui_state_t* state) {
    if (!screen || !state) return;
    
    // Update connection status
    if (state->wifi_connected && !state->wifi_ap_mode) {
        lv_label_set_text(status_label, "Connected! Redirecting...");
        lv_obj_set_style_text_color(status_label, COLOR_SUCCESS, 0);
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(status_label, "Waiting for connection...");
        lv_obj_set_style_text_color(status_label, COLOR_TEXT_MUTED, 0);
        lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void screen_setup_set_ap_info(const char* ssid, const char* password, const char* ip) {
    if (ssid_label && ssid) {
        lv_label_set_text(ssid_label, ssid);
    }
    if (password_label && password) {
        lv_label_set_text(password_label, password);
    }
    if (ip_label && ip) {
        char url[32];
        snprintf(url, sizeof(url), "http://%s", ip);
        lv_label_set_text(ip_label, url);
    }
}

