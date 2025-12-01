/**
 * BrewOS Shot Complete Screen Implementation
 * 
 * Shot summary display after brewing completes
 * Optimized for 480x480 round display
 */

#include "platform/platform.h"
#include "ui/screen_complete.h"
#include "display/theme.h"
#include "display/display_config.h"

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* checkmark = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* time_value = nullptr;
static lv_obj_t* weight_value = nullptr;
static lv_obj_t* ratio_value = nullptr;
static lv_obj_t* flow_value = nullptr;
static lv_obj_t* hint_label = nullptr;

static complete_dismiss_callback_t dismiss_callback = nullptr;

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_complete_create(void) {
    LOG_I("Creating complete screen...");
    
    // Create screen with dark background
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Checkmark Icon ===
    checkmark = lv_label_create(screen);
    lv_label_set_text(checkmark, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(checkmark, FONT_XLARGE, 0);
    lv_obj_set_style_text_color(checkmark, COLOR_SUCCESS, 0);
    lv_obj_align(checkmark, LV_ALIGN_CENTER, 0, -130);
    
    // === Title ===
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Shot Complete");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -80);
    
    // === Compact Stats Card ===
    lv_obj_t* card = lv_obj_create(screen);
    lv_obj_set_size(card, 260, 155);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(card, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, RADIUS_NORMAL, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Time row
    lv_obj_t* time_row = lv_obj_create(card);
    lv_obj_remove_style_all(time_row);
    lv_obj_set_size(time_row, lv_pct(100), 32);
    lv_obj_align(time_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "Time");
    lv_obj_set_style_text_font(time_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(time_label, COLOR_TEXT_MUTED, 0);
    
    time_value = lv_label_create(time_row);
    lv_label_set_text(time_value, "00:00");
    lv_obj_set_style_text_font(time_value, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(time_value, COLOR_TEXT_PRIMARY, 0);
    
    // Weight row
    lv_obj_t* weight_row = lv_obj_create(card);
    lv_obj_remove_style_all(weight_row);
    lv_obj_set_size(weight_row, lv_pct(100), 32);
    lv_obj_align(weight_row, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_flex_flow(weight_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(weight_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* weight_label = lv_label_create(weight_row);
    lv_label_set_text(weight_label, "Weight");
    lv_obj_set_style_text_font(weight_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(weight_label, COLOR_TEXT_MUTED, 0);
    
    weight_value = lv_label_create(weight_row);
    lv_label_set_text(weight_value, "0.0g");
    lv_obj_set_style_text_font(weight_value, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(weight_value, COLOR_ACCENT_AMBER, 0);
    
    // Ratio row
    lv_obj_t* ratio_row = lv_obj_create(card);
    lv_obj_remove_style_all(ratio_row);
    lv_obj_set_size(ratio_row, lv_pct(100), 32);
    lv_obj_align(ratio_row, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_flex_flow(ratio_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ratio_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* ratio_label = lv_label_create(ratio_row);
    lv_label_set_text(ratio_label, "Ratio");
    lv_obj_set_style_text_font(ratio_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(ratio_label, COLOR_TEXT_MUTED, 0);
    
    ratio_value = lv_label_create(ratio_row);
    lv_label_set_text(ratio_value, "1:0.0");
    lv_obj_set_style_text_font(ratio_value, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(ratio_value, COLOR_TEXT_PRIMARY, 0);
    
    // Flow row
    lv_obj_t* flow_row = lv_obj_create(card);
    lv_obj_remove_style_all(flow_row);
    lv_obj_set_size(flow_row, lv_pct(100), 32);
    lv_obj_align(flow_row, LV_ALIGN_TOP_MID, 0, 105);
    lv_obj_set_flex_flow(flow_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(flow_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* flow_label = lv_label_create(flow_row);
    lv_label_set_text(flow_label, "Avg Flow");
    lv_obj_set_style_text_font(flow_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(flow_label, COLOR_TEXT_MUTED, 0);
    
    flow_value = lv_label_create(flow_row);
    lv_label_set_text(flow_value, "0.0 ml/s");
    lv_obj_set_style_text_font(flow_value, FONT_SMALL, 0);
    lv_obj_set_style_text_color(flow_value, COLOR_TEXT_SECONDARY, 0);
    
    // === Hint at bottom ===
    hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, "Press to continue");
    lv_obj_set_style_text_font(hint_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(hint_label, LV_ALIGN_CENTER, 0, 135);
    
    LOG_I("Complete screen created");
    return screen;
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_complete_update(uint32_t time_ms, float weight, float dose, float avg_flow) {
    if (!screen) return;
    
    // Update time
    uint32_t secs = time_ms / 1000;
    uint32_t mins = secs / 60;
    secs = secs % 60;
    
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02lu:%02lu", (unsigned long)mins, (unsigned long)secs);
    lv_label_set_text(time_value, time_str);
    
    // Update weight
    char weight_str[16];
    snprintf(weight_str, sizeof(weight_str), "%.1fg", weight);
    lv_label_set_text(weight_value, weight_str);
    
    // Update ratio
    if (dose > 0) {
        float ratio = weight / dose;
        char ratio_str[16];
        snprintf(ratio_str, sizeof(ratio_str), "1:%.1f", ratio);
        lv_label_set_text(ratio_value, ratio_str);
    } else {
        lv_label_set_text(ratio_value, "1:--");
    }
    
    // Update flow
    char flow_str[16];
    snprintf(flow_str, sizeof(flow_str), "%.1f ml/s", avg_flow);
    lv_label_set_text(flow_value, flow_str);
}

void screen_complete_set_dismiss_callback(complete_dismiss_callback_t callback) {
    dismiss_callback = callback;
}

