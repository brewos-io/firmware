#ifndef VALIDATION_H
#define VALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t

/**
 * Validation Utility Module
 * 
 * Provides centralized input validation with consistent error codes.
 * Thread-safe, no side effects.
 */

// =============================================================================
// Error Codes
// =============================================================================

typedef enum {
    VALIDATION_OK = 0,
    VALIDATION_ERROR_NULL_PTR,
    VALIDATION_ERROR_OUT_OF_RANGE,
    VALIDATION_ERROR_INVALID_TARGET,
    VALIDATION_ERROR_INVALID_TYPE,
    VALIDATION_ERROR_BUFFER_TOO_SMALL,
    VALIDATION_ERROR_INVALID_CONFIG
} validation_result_t;

// =============================================================================
// Temperature Validation
// =============================================================================

/**
 * Validate temperature value
 * @param temp Temperature in decicelsius (0.1C units)
 * @param min_temp Minimum valid temperature in decicelsius
 * @param max_temp Maximum valid temperature in decicelsius
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_temperature(int16_t temp, int16_t min_temp, int16_t max_temp);

/**
 * Validate brew/steam target index
 * @param target Target index (0=brew, 1=steam)
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_setpoint_target(uint8_t target);

// =============================================================================
// PID Validation
// =============================================================================

/**
 * Validate PID gains
 * @param kp Proportional gain (scaled by 100)
 * @param ki Integral gain (scaled by 100)
 * @param kd Derivative gain (scaled by 100)
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_pid_gains(uint16_t kp, uint16_t ki, uint16_t kd);

// =============================================================================
// Electrical Configuration Validation
// =============================================================================

/**
 * Validate electrical voltage
 * @param voltage Voltage in volts
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_voltage(uint16_t voltage);

/**
 * Validate electrical current
 * @param current Current in amps
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_current(float current);

// =============================================================================
// Buffer Validation
// =============================================================================

/**
 * Validate buffer copy operation
 * @param dest Destination pointer
 * @param src Source pointer
 * @param size Size to copy
 * @param dest_size Destination buffer size
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_buffer_copy(const void* dest, const void* src, 
                                        size_t size, size_t dest_size);

// =============================================================================
// Pre-infusion Validation
// =============================================================================

/**
 * Validate pre-infusion timing parameters
 * @param on_time_ms Pump on time in milliseconds
 * @param pause_time_ms Pause time in milliseconds
 * @return VALIDATION_OK if valid, error code otherwise
 */
validation_result_t validate_preinfusion_timing(uint16_t on_time_ms, uint16_t pause_time_ms);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get human-readable error message
 * @param result Validation result code
 * @return Error message string (never NULL)
 */
const char* validation_error_string(validation_result_t result);

#endif // VALIDATION_H
