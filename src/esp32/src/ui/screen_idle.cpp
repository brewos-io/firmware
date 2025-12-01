/**
 * BrewOS Idle Screen Implementation
 * 
 * Power on screen with heating strategy selection
 * Optimized for 480x480 round display with logo
 */

#include "platform/platform.h"
#include "ui/screen_idle.h"
#include "display/theme.h"
#include "display/display_config.h"

// Strategy names (matching protocol_defs.h values)
static const char* strategy_names[] = {
    "Brew Only",      // 0
    "Sequential",     // 1
    "Parallel",       // 2
    "Smart Stagger"   // 3
};

static const char* strategy_descriptions[] = {
    "Heat brew boiler only",
    "Brew then steam",
    "Heat both at once",
    "Power efficient"
};

// Map array index to protocol strategy value (note: value 3 is unused)
static const uint8_t strategy_values[] = {
    0,  // BREW_ONLY
    1,  // SEQUENTIAL
    2,  // PARALLEL
    3   // SMART_STAGGER
};

#define STRATEGY_COUNT 4

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* logo_img = nullptr;
static lv_obj_t* power_icon = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* strategy_name_label = nullptr;
static lv_obj_t* strategy_desc_label = nullptr;
static lv_obj_t* hint_label = nullptr;
static lv_obj_t* dots_container = nullptr;
static lv_obj_t* strategy_dots[STRATEGY_COUNT] = {nullptr};

// State
static int selected_index = 0;
static idle_turn_on_callback_t turn_on_callback = nullptr;

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_idle_create(void) {
    LOG_I("Creating idle screen...");
    
    // Create screen with dark background
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Logo at top (or power icon as fallback) ===
    #ifdef LV_USE_FS_STDIO
    // Try to load logo image
    logo_img = lv_img_create(screen);
    lv_img_set_src(logo_img, "S:/logo-icon.png");
    lv_obj_set_size(logo_img, 80, 80);
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, -100);
    #else
    // Fallback: Power icon with animation
    power_icon = lv_label_create(screen);
    lv_label_set_text(power_icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(power_icon, FONT_XLARGE, 0);
    lv_obj_set_style_text_color(power_icon, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(power_icon, LV_ALIGN_CENTER, 0, -100);
    
    // Add subtle pulse animation
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, power_icon);
    lv_anim_set_values(&anim, LV_OPA_60, LV_OPA_COVER);
    lv_anim_set_time(&anim, 1200);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_playback_time(&anim, 1200);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
    });
    lv_anim_start(&anim);
    #endif
    
    // === Title ===
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Press to Start");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -20);
    
    // === Strategy Name ===
    strategy_name_label = lv_label_create(screen);
    lv_label_set_text(strategy_name_label, strategy_names[selected_index]);
    lv_obj_set_style_text_font(strategy_name_label, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(strategy_name_label, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(strategy_name_label, LV_ALIGN_CENTER, 0, 30);
    
    // === Strategy Description ===
    strategy_desc_label = lv_label_create(screen);
    lv_label_set_text(strategy_desc_label, strategy_descriptions[selected_index]);
    lv_obj_set_style_text_font(strategy_desc_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(strategy_desc_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(strategy_desc_label, LV_ALIGN_CENTER, 0, 58);
    
    // === Dots indicator ===
    dots_container = lv_obj_create(screen);
    lv_obj_remove_style_all(dots_container);
    lv_obj_set_size(dots_container, STRATEGY_COUNT * 18, 16);
    lv_obj_align(dots_container, LV_ALIGN_CENTER, 0, 95);
    lv_obj_set_flex_flow(dots_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    for (int i = 0; i < STRATEGY_COUNT; i++) {
        strategy_dots[i] = lv_obj_create(dots_container);
        lv_obj_set_size(strategy_dots[i], 8, 8);
        lv_obj_set_style_radius(strategy_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(strategy_dots[i], 0, 0);
        lv_obj_set_style_pad_left(strategy_dots[i], 3, 0);
        lv_obj_set_style_pad_right(strategy_dots[i], 3, 0);
        
        if (i == selected_index) {
            lv_obj_set_style_bg_color(strategy_dots[i], COLOR_ACCENT_AMBER, 0);
        } else {
            lv_obj_set_style_bg_color(strategy_dots[i], COLOR_BG_ELEVATED, 0);
        }
    }
    
    // === Hint at bottom ===
    hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, LV_SYMBOL_LOOP " Rotate to select");
    lv_obj_set_style_text_font(hint_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(hint_label, LV_ALIGN_CENTER, 0, 140);
    
    // === Make screen focusable for encoder input ===
    lv_group_t* group = lv_group_get_default();
    if (group) {
        lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
        lv_group_add_obj(group, screen);
        
        // Handle encoder events
        lv_obj_add_event_cb(screen, [](lv_event_t* e) {
            lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_KEY) {
                uint32_t key = lv_event_get_key(e);
                if (key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
                    screen_idle_select_strategy(selected_index + 1);
                } else if (key == LV_KEY_LEFT || key == LV_KEY_PREV) {
                    screen_idle_select_strategy(selected_index - 1);
                }
            }
        }, LV_EVENT_KEY, NULL);
        
        lv_group_set_editing(group, true);
    }
    
    LOG_I("Idle screen created");
    return screen;
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_idle_update(const ui_state_t* state) {
    if (!screen || !state) return;
    
    // Update selected strategy from state if not idle (e.g., coming from settings)
    // The current selection is maintained by the screen itself during rotation
}

void screen_idle_select_strategy(int index) {
    // Clamp index
    if (index < 0) index = STRATEGY_COUNT - 1;
    if (index >= STRATEGY_COUNT) index = 0;
    
    selected_index = index;
    
    // Update UI
    if (strategy_name_label) {
        lv_label_set_text(strategy_name_label, strategy_names[selected_index]);
    }
    if (strategy_desc_label) {
        lv_label_set_text(strategy_desc_label, strategy_descriptions[selected_index]);
    }
    
    // Update dots
    for (int i = 0; i < STRATEGY_COUNT; i++) {
        if (strategy_dots[i]) {
            if (i == selected_index) {
                lv_obj_set_style_bg_color(strategy_dots[i], COLOR_ACCENT_AMBER, 0);
            } else {
                lv_obj_set_style_bg_color(strategy_dots[i], COLOR_BG_ELEVATED, 0);
            }
        }
    }
}

heating_strategy_t screen_idle_get_selected_strategy(void) {
    if (selected_index >= 0 && selected_index < STRATEGY_COUNT) {
        return (heating_strategy_t)strategy_values[selected_index];
    }
    return HEAT_BREW_ONLY;
}

void screen_idle_set_turn_on_callback(idle_turn_on_callback_t callback) {
    turn_on_callback = callback;
}

