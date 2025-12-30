/**
 * Validation Utility Module Implementation
 * 
 * Centralized input validation with consistent error handling.
 */

#include "validation.h"
#include <stddef.h>

// =============================================================================
// Constants
// =============================================================================

// Temperature limits (in decicelsius)
#define TEMP_ABSOLUTE_MIN    0      // 0.0°C
#define TEMP_ABSOLUTE_MAX    2000   // 200.0°C
#define TEMP_BREW_MAX        1300   // 130.0°C (safety limit)
#define TEMP_STEAM_MAX       1650   // 165.0°C (safety limit)

// PID limits (scaled by 100)
#define PID_GAIN_MAX         10000  // 100.00 (very aggressive)

// Electrical limits
#define VOLTAGE_MIN          100    // 100V
#define VOLTAGE_MAX          250    // 250V
#define CURRENT_MIN          1.0f   // 1A
#define CURRENT_MAX          50.0f  // 50A

// Pre-infusion limits
#define PREINFUSION_ON_MAX   10000  // 10 seconds
#define PREINFUSION_PAUSE_MAX 30000 // 30 seconds

// =============================================================================
// Temperature Validation
// =============================================================================

validation_result_t validate_temperature(int16_t temp, int16_t min_temp, int16_t max_temp) {
    if (temp < min_temp || temp > max_temp) {
        return VALIDATION_ERROR_OUT_OF_RANGE;
    }
    return VALIDATION_OK;
}

validation_result_t validate_setpoint_target(uint8_t target) {
    if (target > 1) {  // 0=brew, 1=steam
        return VALIDATION_ERROR_INVALID_TARGET;
    }
    return VALIDATION_OK;
}

// =============================================================================
// PID Validation
// =============================================================================

validation_result_t validate_pid_gains(uint16_t kp, uint16_t ki, uint16_t kd) {
    if (kp > PID_GAIN_MAX || ki > PID_GAIN_MAX || kd > PID_GAIN_MAX) {
        return VALIDATION_ERROR_OUT_OF_RANGE;
    }
    return VALIDATION_OK;
}

// =============================================================================
// Electrical Configuration Validation
// =============================================================================

validation_result_t validate_voltage(uint16_t voltage) {
    if (voltage < VOLTAGE_MIN || voltage > VOLTAGE_MAX) {
        return VALIDATION_ERROR_OUT_OF_RANGE;
    }
    return VALIDATION_OK;
}

validation_result_t validate_current(float current) {
    if (current < CURRENT_MIN || current > CURRENT_MAX) {
        return VALIDATION_ERROR_OUT_OF_RANGE;
    }
    return VALIDATION_OK;
}

// =============================================================================
// Buffer Validation
// =============================================================================

validation_result_t validate_buffer_copy(const void* dest, const void* src, 
                                        size_t size, size_t dest_size) {
    if (dest == NULL || src == NULL) {
        return VALIDATION_ERROR_NULL_PTR;
    }
    if (size > dest_size) {
        return VALIDATION_ERROR_BUFFER_TOO_SMALL;
    }
    return VALIDATION_OK;
}

// =============================================================================
// Pre-infusion Validation
// =============================================================================

validation_result_t validate_preinfusion_timing(uint16_t on_time_ms, uint16_t pause_time_ms) {
    if (on_time_ms > PREINFUSION_ON_MAX) {
        return VALIDATION_ERROR_OUT_OF_RANGE;
    }
    if (pause_time_ms > PREINFUSION_PAUSE_MAX) {
        return VALIDATION_ERROR_OUT_OF_RANGE;
    }
    return VALIDATION_OK;
}

// =============================================================================
// Utility Functions
// =============================================================================

const char* validation_error_string(validation_result_t result) {
    switch (result) {
        case VALIDATION_OK:
            return "OK";
        case VALIDATION_ERROR_NULL_PTR:
            return "Null pointer";
        case VALIDATION_ERROR_OUT_OF_RANGE:
            return "Value out of range";
        case VALIDATION_ERROR_INVALID_TARGET:
            return "Invalid target";
        case VALIDATION_ERROR_INVALID_TYPE:
            return "Invalid type";
        case VALIDATION_ERROR_BUFFER_TOO_SMALL:
            return "Buffer too small";
        case VALIDATION_ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        default:
            return "Unknown error";
    }
}
