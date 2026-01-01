/**
 * Status Change Detector
 * 
 * Detects meaningful changes in device status to avoid sending redundant updates
 * to cloud and MQTT. Only sends updates when data actually changes or on periodic
 * heartbeats to maintain connectivity.
 */

#ifndef STATUS_CHANGE_DETECTOR_H
#define STATUS_CHANGE_DETECTOR_H

#include "ui/ui.h"
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Change Detection Thresholds
// =============================================================================

// Temperature change threshold (degrees Celsius)
// Only trigger update if temperature changes by this amount
#define STATUS_TEMP_THRESHOLD 0.5f

// Pressure change threshold (bar)
#define STATUS_PRESSURE_THRESHOLD 0.1f

// Power change threshold (watts)
#define STATUS_POWER_THRESHOLD 10.0f

// Weight change threshold (grams)
#define STATUS_WEIGHT_THRESHOLD 0.5f

// Flow rate change threshold (ml/s)
#define STATUS_FLOW_RATE_THRESHOLD 0.1f

// =============================================================================
// Status Change Detector Class
// =============================================================================

class StatusChangeDetector {
public:
    StatusChangeDetector();
    
    /**
     * Check if the current status has changed meaningfully from the previous status
     * @param current Current device status
     * @return true if any meaningful field changed, false otherwise
     */
    bool hasChanged(const ui_state_t& current);
    
    /**
     * Reset the detector (useful after reconnection or initialization)
     * Forces next check to return true
     */
    void reset();
    
    /**
     * Enable/disable debug logging of what changed
     */
    void setDebug(bool enabled) { _debug = enabled; }

private:
    // Previous status values for comparison
    ui_state_t _previous;
    bool _initialized;
    bool _debug;
    
    /**
     * Compare two float values with a threshold
     */
    bool floatChanged(float current, float previous, float threshold) const;
};

#endif // STATUS_CHANGE_DETECTOR_H

