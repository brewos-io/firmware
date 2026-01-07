/**
 * BrewOS Rotary Encoder Driver
 * 
 * Uses ESP32_Knob and ESP32_Button libraries for reliable input handling.
 * Timer-based polling with proper debouncing.
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "display_config.h"

// Forward declarations for library classes
class ESP_Knob;
class Button;

// =============================================================================
// Button State Enum
// =============================================================================

typedef enum {
    BTN_RELEASED,
    BTN_PRESSED,
    BTN_LONG_PRESSED,
    BTN_DOUBLE_PRESSED
} button_state_t;

// =============================================================================
// Encoder Event Callback
// =============================================================================

typedef void (*encoder_callback_t)(int32_t diff, button_state_t btn);

// =============================================================================
// Encoder Driver Class
// =============================================================================

class Encoder {
public:
    Encoder();
    ~Encoder();
    
    /**
     * Initialize encoder hardware and LVGL input device
     */
    bool begin();
    
    /**
     * Update encoder state - call this in main loop
     */
    void update();
    
    /**
     * Get current encoder position (relative since last read)
     */
    int32_t getPosition() const { return _position; }
    
    /**
     * Get current button state
     */
    button_state_t getButtonState() const { return _buttonState; }
    
    /**
     * Check if button is currently pressed
     */
    bool isPressed() const { return _buttonPressed; }
    
    /**
     * Check if button was long-pressed
     */
    bool wasLongPressed() const { return _buttonState == BTN_LONG_PRESSED; }
    
    /**
     * Check if button was double-pressed
     */
    bool wasDoublePressed() const { return _buttonState == BTN_DOUBLE_PRESSED; }
    
    /**
     * Reset encoder position to zero
     */
    void resetPosition();
    
    /**
     * Clear button state (after handling)
     */
    void clearButtonState() { _buttonState = BTN_RELEASED; }
    
    /**
     * Set callback for encoder events
     */
    void setCallback(encoder_callback_t callback) { _callback = callback; }
    
    /**
     * Get LVGL input device (for advanced usage)
     */
    lv_indev_t* getInputDevice() const { return _indev; }
    
    // Event handlers (called by library callbacks)
    void onKnobLeft(int count);
    void onKnobRight(int count);
    void onButtonSingleClick();
    void onButtonDoubleClick();
    void onButtonLongPress();
    
private:
    // LVGL input device
    lv_indev_t* _indev;
    lv_indev_drv_t _indevDrv;
    
    // ESP32 Knob and Button library instances
    ESP_Knob* _knob;
    Button* _button;
    
    // Encoder state
    volatile int32_t _position;
    int32_t _lastReportedPosition;    // For custom callback
    int32_t _lastLvglPosition;        // For LVGL input device (separate tracking)
    
    // Button state
    bool _buttonPressed;
    button_state_t _buttonState;
    button_state_t _lastReportedButtonState;
    
    // Callback
    encoder_callback_t _callback;
    
    // Static callbacks for LVGL
    static void readCallback(lv_indev_drv_t* drv, lv_indev_data_t* data);
};

// Global encoder instance (only when screen is enabled)
#if ENABLE_SCREEN
extern Encoder encoder;
#endif

#endif // ENCODER_H
