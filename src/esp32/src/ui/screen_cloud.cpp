/**
 * BrewOS Cloud Pairing Screen Implementation
 * 
 * Shows QR code and pairing code for cloud integration
 * Optimized for 480x480 round display
 */

#include "platform/platform.h"
#include "ui/screen_cloud.h"
#include "display/theme.h"
#include "display/display_config.h"

// Check if QR code support is available
#if LV_USE_QRCODE
#define HAS_QRCODE 1
#else
#define HAS_QRCODE 0
#endif

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* qr_container = nullptr;
static lv_obj_t* qr_code = nullptr;
static lv_obj_t* qr_placeholder = nullptr;
static lv_obj_t* device_id_label = nullptr;
static lv_obj_t* code_label = nullptr;
static lv_obj_t* expires_label = nullptr;
static lv_obj_t* refresh_btn = nullptr;
static lv_obj_t* spinner = nullptr;
static lv_obj_t* error_label = nullptr;

// Callback
static cloud_refresh_callback_t refresh_callback = nullptr;

// Current pairing URL for QR
static char current_url[256] = {0};

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_cloud_create(void) {
    LOG_I("Creating cloud pairing screen...");
    
    // Create screen with dark background
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Title ===
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Cloud Pairing");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -150);
    
    // === QR Code container ===
    qr_container = lv_obj_create(screen);
    lv_obj_set_size(qr_container, 140, 140);
    lv_obj_align(qr_container, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(qr_container, lv_color_white(), 0);
    lv_obj_set_style_radius(qr_container, 8, 0);
    lv_obj_set_style_border_width(qr_container, 0, 0);
    lv_obj_set_style_pad_all(qr_container, 8, 0);
    lv_obj_clear_flag(qr_container, LV_OBJ_FLAG_SCROLLABLE);
    
#if HAS_QRCODE
    // QR Code widget (if available)
    qr_code = lv_qrcode_create(qr_container, 120, lv_color_black(), lv_color_white());
    lv_obj_center(qr_code);
    lv_qrcode_update(qr_code, "brewos://pair", strlen("brewos://pair"));
#else
    // Placeholder when QR code not available
    qr_placeholder = lv_label_create(qr_container);
    lv_label_set_text(qr_placeholder, LV_SYMBOL_IMAGE "\nQR");
    lv_obj_set_style_text_font(qr_placeholder, FONT_XLARGE, 0);
    lv_obj_set_style_text_color(qr_placeholder, lv_color_black(), 0);
    lv_obj_set_style_text_align(qr_placeholder, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(qr_placeholder);
#endif
    
    // === Loading spinner (hidden by default) ===
    spinner = lv_spinner_create(screen, 1000, 60);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_arc_color(spinner, COLOR_ACCENT_AMBER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 8, 0);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    
    // === Error label (hidden by default) ===
    error_label = lv_label_create(screen);
    lv_label_set_text(error_label, "");
    lv_obj_set_style_text_font(error_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(error_label, COLOR_ERROR, 0);
    lv_obj_set_style_text_align(error_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(error_label, LV_ALIGN_CENTER, 0, -50);
    lv_obj_add_flag(error_label, LV_OBJ_FLAG_HIDDEN);
    
    // === Device ID ===
    device_id_label = lv_label_create(screen);
    lv_label_set_text(device_id_label, "Device: BRW-XXXXXXXX");
    lv_obj_set_style_text_font(device_id_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(device_id_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(device_id_label, LV_ALIGN_CENTER, 0, 40);
    
    // === Pairing Code (large, prominent) ===
    code_label = lv_label_create(screen);
    lv_label_set_text(code_label, "--------");
    lv_obj_set_style_text_font(code_label, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(code_label, COLOR_ACCENT_AMBER, 0);
    lv_obj_set_style_text_letter_space(code_label, 2, 0);
    lv_obj_align(code_label, LV_ALIGN_CENTER, 0, 70);
    
    // === Expires label ===
    expires_label = lv_label_create(screen);
    lv_label_set_text(expires_label, "Scan QR or enter code");
    lv_obj_set_style_text_font(expires_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(expires_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(expires_label, LV_ALIGN_CENTER, 0, 100);
    
    // === Refresh button ===
    refresh_btn = lv_btn_create(screen);
    lv_obj_set_size(refresh_btn, 120, 36);
    lv_obj_align(refresh_btn, LV_ALIGN_CENTER, 0, 145);
    lv_obj_set_style_bg_color(refresh_btn, COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(refresh_btn, 18, 0);
    lv_obj_set_style_border_width(refresh_btn, 1, 0);
    lv_obj_set_style_border_color(refresh_btn, COLOR_ACCENT_AMBER, 0);
    
    lv_obj_t* btn_label = lv_label_create(refresh_btn);
    lv_label_set_text(btn_label, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_set_style_text_font(btn_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(btn_label);
    
    // Add to encoder group
    lv_group_t* group = lv_group_get_default();
    if (group) {
        lv_group_add_obj(group, refresh_btn);
        
        // Focus style
        lv_obj_set_style_outline_width(refresh_btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(refresh_btn, COLOR_ACCENT_PRIMARY, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(refresh_btn, 2, LV_STATE_FOCUSED);
        
        // Press handler
        lv_obj_add_event_cb(refresh_btn, [](lv_event_t* e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                screen_cloud_select();
            }
        }, LV_EVENT_CLICKED, NULL);
    }
    
    LOG_I("Cloud pairing screen created");
    return screen;
}

// =============================================================================
// Screen Updates
// =============================================================================

void screen_cloud_update(const char* device_id, const char* token, 
                         const char* url, uint32_t expires_in) {
    if (!screen) return;
    
    // Show QR container, hide spinner and error
    lv_obj_clear_flag(qr_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Update device ID
    if (device_id && device_id_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Device: %s", device_id);
        lv_label_set_text(device_id_label, buf);
    }
    
    // Update pairing code (show first 8 chars of token)
    if (token && code_label) {
        char short_code[12];
        strncpy(short_code, token, 8);
        short_code[8] = '\0';
        lv_label_set_text(code_label, short_code);
    }
    
    // Update QR code (if available)
    if (url) {
        strncpy(current_url, url, sizeof(current_url) - 1);
        current_url[sizeof(current_url) - 1] = '\0';
#if HAS_QRCODE
        if (qr_code) {
            lv_qrcode_update(qr_code, current_url, strlen(current_url));
        }
#endif
    }
    
    // Update expires label
    if (expires_label) {
        if (expires_in > 0) {
            uint32_t mins = expires_in / 60;
            uint32_t secs = expires_in % 60;
            char buf[32];
            snprintf(buf, sizeof(buf), "Expires in %lu:%02lu", 
                    (unsigned long)mins, (unsigned long)secs);
            lv_label_set_text(expires_label, buf);
            lv_obj_set_style_text_color(expires_label, COLOR_TEXT_MUTED, 0);
        } else {
            lv_label_set_text(expires_label, "Code expired - refresh");
            lv_obj_set_style_text_color(expires_label, COLOR_WARNING, 0);
        }
    }
}

void screen_cloud_show_loading(void) {
    if (!screen) return;
    
    // Hide QR and error, show spinner
    lv_obj_add_flag(qr_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(error_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    
    lv_label_set_text(code_label, "--------");
    lv_label_set_text(expires_label, "Generating...");
    lv_obj_set_style_text_color(expires_label, COLOR_TEXT_MUTED, 0);
}

void screen_cloud_show_error(const char* message) {
    if (!screen) return;
    
    // Hide QR and spinner, show error
    lv_obj_add_flag(qr_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(error_label, LV_OBJ_FLAG_HIDDEN);
    
    if (message && error_label) {
        lv_label_set_text(error_label, message);
    }
    
    lv_label_set_text(code_label, "--------");
    lv_label_set_text(expires_label, "Press refresh to try again");
    lv_obj_set_style_text_color(expires_label, COLOR_TEXT_MUTED, 0);
}

// =============================================================================
// Interaction Handlers
// =============================================================================

void screen_cloud_encoder(int direction) {
    // Only one focusable element (refresh button)
    // Could add more navigation in the future
}

void screen_cloud_select(void) {
    if (refresh_callback) {
        screen_cloud_show_loading();
        refresh_callback();
    }
}

void screen_cloud_set_refresh_callback(cloud_refresh_callback_t callback) {
    refresh_callback = callback;
}

