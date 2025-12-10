/**
 * BrewOS Scale Pairing Screen Implementation
 * 
 * BLE scale discovery, connection, and status display
 */

#include "platform/platform.h"
#include "ui/screen_scale.h"
#include "display/theme.h"
#include "display/display_config.h"
#include "scale/scale_manager.h"

// Static elements
static lv_obj_t* screen = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* status_icon = nullptr;
static lv_obj_t* status_label = nullptr;
static lv_obj_t* weight_label = nullptr;
static lv_obj_t* scale_list = nullptr;
static lv_obj_t* spinner = nullptr;
static lv_obj_t* action_btn = nullptr;
static lv_obj_t* hint_label = nullptr;

// State
static scale_screen_state_t current_state = SCALE_SCREEN_IDLE;
static int selected_index = 0;

// Forward declarations
static void update_status_display(void);
static void update_list(void);
static void create_list_item(const scale_info_t& info, int index);

// =============================================================================
// Screen Creation
// =============================================================================

lv_obj_t* screen_scale_create(void) {
    LOG_I("Creating scale pairing screen...");
    
    // Create screen
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BG_DARK, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Bluetooth Scale");
    lv_obj_set_style_text_font(title_label, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 60);
    
    // Status icon (Bluetooth symbol)
    status_icon = lv_label_create(screen);
    lv_label_set_text(status_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(status_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(status_icon, COLOR_INFO, 0);
    lv_obj_align(status_icon, LV_ALIGN_TOP_MID, 0, 100);
    
    // Status label
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "No scale connected");
    lv_obj_set_style_text_font(status_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(status_label, COLOR_TEXT_MUTED, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 160);
    
    // Weight display (shown when connected)
    weight_label = lv_label_create(screen);
    lv_label_set_text(weight_label, "0.0g");
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(weight_label, COLOR_ACCENT_AMBER, 0);
    lv_obj_align(weight_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_flag(weight_label, LV_OBJ_FLAG_HIDDEN);
    
    // Spinner (shown during scan/connect)
    spinner = lv_spinner_create(screen, 1000, 60);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_arc_color(spinner, COLOR_INFO, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    
    // Scale list (for discovered devices)
    scale_list = lv_obj_create(screen);
    lv_obj_set_size(scale_list, 280, 180);
    lv_obj_align(scale_list, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(scale_list, COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(scale_list, 12, 0);
    lv_obj_set_style_border_width(scale_list, 0, 0);
    lv_obj_set_style_pad_all(scale_list, 8, 0);
    lv_obj_set_flex_flow(scale_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scale_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
    
    // Action button
    action_btn = lv_btn_create(screen);
    lv_obj_set_size(action_btn, 160, 44);
    lv_obj_align(action_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(action_btn, COLOR_INFO, 0);
    lv_obj_set_style_radius(action_btn, 22, 0);
    
    lv_obj_t* btn_label = lv_label_create(action_btn);
    lv_label_set_text(btn_label, "Scan for Scales");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_NORMAL, 0);
    
    // Hint label
    hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, "Press to scan • Long press to exit");
    lv_obj_set_style_text_font(hint_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint_label, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -60);
    
    // Initialize state
    current_state = SCALE_SCREEN_IDLE;
    update_status_display();
    
    LOG_I("Scale pairing screen created");
    return screen;
}

// =============================================================================
// Status Display
// =============================================================================

static void update_status_display(void) {
    if (!screen) return;
    
    lv_obj_t* btn_label = lv_obj_get_child(action_btn, 0);
    
    if (scaleManager && scaleManager->isConnected()) {
        // Connected state
        lv_obj_set_style_text_color(status_icon, COLOR_SUCCESS, 0);
        lv_label_set_text(status_label, scaleManager ? scaleManager->getScaleName() : "N/A");
        lv_obj_clear_flag(weight_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
        
        lv_label_set_text(btn_label, "Disconnect");
        lv_obj_set_style_bg_color(action_btn, COLOR_ERROR, 0);
        
        lv_label_set_text(hint_label, "Press to disconnect • Tare with double-press");
        current_state = SCALE_SCREEN_IDLE;
    }
    else if (scaleManager && scaleManager->isScanning()) {
        // Scanning state
        lv_obj_set_style_text_color(status_icon, COLOR_INFO, 0);
        lv_label_set_text(status_label, "Scanning for scales...");
        lv_obj_add_flag(weight_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
        
        lv_label_set_text(btn_label, "Stop Scan");
        lv_obj_set_style_bg_color(action_btn, COLOR_WARNING, 0);
        
        lv_label_set_text(hint_label, "Press to stop • Long press to exit");
        current_state = SCALE_SCREEN_SCANNING;
    }
    else if (current_state == SCALE_SCREEN_LIST) {
        // Show discovered scales
        lv_obj_set_style_text_color(status_icon, COLOR_INFO, 0);
        
        static const std::vector<scale_info_t> empty_devices;
        const auto& devices = scaleManager ? scaleManager->getDiscoveredScales() : empty_devices;
        if (devices.empty()) {
            lv_label_set_text(status_label, "No scales found");
            lv_obj_add_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "Found %d scale(s)", (int)devices.size());
            lv_label_set_text(status_label, buf);
            lv_obj_clear_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
        }
        
        lv_obj_add_flag(weight_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        
        lv_label_set_text(btn_label, "Scan Again");
        lv_obj_set_style_bg_color(action_btn, COLOR_INFO, 0);
        
        lv_label_set_text(hint_label, "Rotate to select • Press to connect");
    }
    else if (current_state == SCALE_SCREEN_CONNECTING) {
        // Connecting state
        lv_obj_set_style_text_color(status_icon, COLOR_WARNING, 0);
        lv_label_set_text(status_label, "Connecting...");
        lv_obj_add_flag(weight_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
        
        lv_obj_add_flag(action_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(hint_label, "Please wait...");
    }
    else {
        // Idle/disconnected state
        lv_obj_set_style_text_color(status_icon, COLOR_TEXT_MUTED, 0);
        lv_label_set_text(status_label, "No scale connected");
        lv_obj_add_flag(weight_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(scale_list, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(action_btn, LV_OBJ_FLAG_HIDDEN);
        
        lv_label_set_text(btn_label, "Scan for Scales");
        lv_obj_set_style_bg_color(action_btn, COLOR_INFO, 0);
        
        lv_label_set_text(hint_label, "Press to scan • Long press to exit");
        current_state = SCALE_SCREEN_IDLE;
    }
}

// =============================================================================
// List Management
// =============================================================================

static void update_list(void) {
    if (!scale_list) return;
    
    // Clear existing items
    lv_obj_clean(scale_list);
    selected_index = 0;
    
    static const std::vector<scale_info_t> empty_devices;
    const auto& devices = scaleManager ? scaleManager->getDiscoveredScales() : empty_devices;
    
    for (size_t i = 0; i < devices.size(); i++) {
        create_list_item(devices[i], i);
    }
    
    if (!devices.empty()) {
        current_state = SCALE_SCREEN_LIST;
    }
    
    update_status_display();
}

static void create_list_item(const scale_info_t& info, int index) {
    lv_obj_t* item = lv_obj_create(scale_list);
    lv_obj_set_size(item, 260, 40);
    lv_obj_set_style_bg_color(item, index == selected_index ? COLOR_BG_ELEVATED : COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(item, 8, 0);
    lv_obj_set_style_border_width(item, index == selected_index ? 1 : 0, 0);
    lv_obj_set_style_border_color(item, COLOR_ACCENT_PRIMARY, 0);
    lv_obj_set_style_pad_all(item, 6, 0);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    
    // Scale name
    lv_obj_t* name = lv_label_create(item);
    lv_label_set_text(name, info.name);
    lv_obj_set_style_text_font(name, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(name, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);
    
    // Scale type and RSSI
    char buf[32];
    snprintf(buf, sizeof(buf), "%s • %ddBm", getScaleTypeName(info.type), info.rssi);
    lv_obj_t* detail = lv_label_create(item);
    lv_label_set_text(detail, buf);
    lv_obj_set_style_text_font(detail, FONT_SMALL, 0);
    lv_obj_set_style_text_color(detail, COLOR_TEXT_MUTED, 0);
    lv_obj_align(detail, LV_ALIGN_RIGHT_MID, 0, 0);
}

// =============================================================================
// Screen Update
// =============================================================================

void screen_scale_update(const ui_state_t* state) {
    if (!state || !screen) return;
    
    // Update weight display if connected
    if (scaleManager && scaleManager->isConnected()) {
        scale_state_t scale_state = scaleManager ? scaleManager->getState() : scale_state_t{};
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1fg", scale_state.weight);
        lv_label_set_text(weight_label, buf);
        
        // Update color based on stability
        lv_obj_set_style_text_color(weight_label, 
            scale_state.stable ? COLOR_ACCENT_AMBER : COLOR_TEXT_MUTED, 0);
    }
    
    // Check for state changes
    static bool was_scanning = false;
    static bool was_connected = false;
    
    bool is_scanning = scaleManager ? scaleManager->isScanning() : false;
    bool is_connected = scaleManager ? scaleManager->isConnected() : false;
    
    if (was_scanning && !is_scanning) {
        // Scan finished - update list
        update_list();
    }
    
    if (was_connected != is_connected) {
        update_status_display();
    }
    
    was_scanning = is_scanning;
    was_connected = is_connected;
}

// =============================================================================
// Encoder Handling
// =============================================================================

void screen_scale_encoder(int direction) {
    if (current_state == SCALE_SCREEN_LIST) {
        static const std::vector<scale_info_t> empty_devices;
        const auto& devices = scaleManager ? scaleManager->getDiscoveredScales() : empty_devices;
        if (devices.empty()) return;
        
        int new_index = selected_index + direction;
        if (new_index < 0) new_index = devices.size() - 1;
        if (new_index >= (int)devices.size()) new_index = 0;
        
        if (new_index != selected_index) {
            selected_index = new_index;
            update_list();
        }
    }
}

bool screen_scale_select(void) {
    switch (current_state) {
        case SCALE_SCREEN_IDLE:
            if (scaleManager && scaleManager->isConnected()) {
                // Disconnect
                if (scaleManager) scaleManager->disconnect();
                update_status_display();
            } else {
                // Start scan
                screen_scale_start_scan();
            }
            return true;
            
        case SCALE_SCREEN_SCANNING:
            // Stop scan
            screen_scale_stop_scan();
            update_list();
            return true;
            
        case SCALE_SCREEN_LIST: {
            // Connect to selected scale
            static const std::vector<scale_info_t> empty_devices;
        const auto& devices = scaleManager ? scaleManager->getDiscoveredScales() : empty_devices;
            if (selected_index >= 0 && selected_index < (int)devices.size()) {
                current_state = SCALE_SCREEN_CONNECTING;
                update_status_display();
                
                if (scaleManager && scaleManager->connectByIndex(selected_index)) {
                    LOG_I("Connecting to scale index %d", selected_index);
                } else {
                    LOG_W("Failed to connect to scale");
                    current_state = SCALE_SCREEN_LIST;
                    update_status_display();
                }
            }
            return true;
        }
            
        case SCALE_SCREEN_CONNECTING:
            // Do nothing while connecting
            return true;
            
        default:
            return false;
    }
}

void screen_scale_start_scan(void) {
    if (scaleManager) {
        scaleManager->clearDiscovered();
        scaleManager->startScan(15000);
    }
    current_state = SCALE_SCREEN_SCANNING;
    update_status_display();
    LOG_I("Scale scan started");
}

void screen_scale_stop_scan(void) {
        if (scaleManager) scaleManager->stopScan();
    current_state = SCALE_SCREEN_IDLE;
    update_status_display();
    LOG_I("Scale scan stopped");
}

void screen_scale_refresh_list(void) {
    update_list();
}

scale_screen_state_t screen_scale_get_state(void) {
    return current_state;
}

