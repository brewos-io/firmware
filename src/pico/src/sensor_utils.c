/**
 * Pico Firmware - Sensor Utility Functions Implementation
 */

#include "sensor_utils.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>  // For qsort

// =============================================================================
// Median Filter Implementation
// =============================================================================

/**
 * Compare function for qsort (ascending order)
 */
static int compare_float(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

void filter_median_init(median_filter_t* filter, float* buffer, uint8_t size) {
    if (!filter || !buffer || size == 0) {
        return;
    }
    
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->count = 0;
    
    // Initialize buffer to zero
    memset(buffer, 0, size * sizeof(float));
}

float filter_median_update(median_filter_t* filter, float value) {
    if (!filter || !filter->buffer || filter->size == 0) {
        return value;  // Return unfiltered if invalid
    }
    
    // Add new value to buffer
    filter->buffer[filter->index] = value;
    filter->index = (filter->index + 1) % filter->size;
    
    // Update count
    if (filter->count < filter->size) {
        filter->count++;
    }
    
    // Calculate median: sort buffer and return middle value
    // Create temporary sorted array for median calculation
    float sorted[16];  // Max reasonable filter size
    if (filter->count > sizeof(sorted) / sizeof(sorted[0])) {
        // Filter too large, return current value
        return value;
    }
    
    // Copy valid samples to temp array
    for (uint8_t i = 0; i < filter->count; i++) {
        sorted[i] = filter->buffer[i];
    }
    
    // Sort to find median
    qsort(sorted, filter->count, sizeof(float), compare_float);
    
    // Return median (middle value)
    uint8_t median_idx = filter->count / 2;
    return sorted[median_idx];
}

void filter_median_reset(median_filter_t* filter) {
    if (!filter) {
        return;
    }
    
    filter->index = 0;
    filter->count = 0;
    
    if (filter->buffer) {
        memset(filter->buffer, 0, filter->size * sizeof(float));
    }
}

// =============================================================================
// Moving Average Filter Implementation
// =============================================================================

void filter_moving_avg_init(moving_avg_filter_t* filter, float* buffer, uint8_t size) {
    if (!filter || !buffer || size == 0) {
        return;
    }
    
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->count = 0;
    filter->sum = 0.0f;
    
    // Initialize buffer to zero
    memset(buffer, 0, size * sizeof(float));
}

float filter_moving_avg_update(moving_avg_filter_t* filter, float value) {
    if (!filter || !filter->buffer || filter->size == 0) {
        return value;  // Return unfiltered if invalid
    }
    
    // Remove old value from sum
    if (filter->count >= filter->size) {
        filter->sum -= filter->buffer[filter->index];
    }
    
    // Add new value
    filter->buffer[filter->index] = value;
    filter->sum += value;
    
    // Update index (circular buffer)
    filter->index = (filter->index + 1) % filter->size;
    
    // Update count
    if (filter->count < filter->size) {
        filter->count++;
    }
    
    // Return average
    return filter->sum / (float)filter->count;
}

void filter_moving_avg_reset(moving_avg_filter_t* filter) {
    if (!filter) {
        return;
    }
    
    filter->index = 0;
    filter->count = 0;
    filter->sum = 0.0f;
    
    if (filter->buffer) {
        memset(filter->buffer, 0, filter->size * sizeof(float));
    }
}

// =============================================================================
// NTC Temperature Conversion Implementation
// =============================================================================

float ntc_adc_to_resistance(uint16_t adc_value, float vref, float r_series) {
    if (adc_value == 0 || adc_value >= 4095) {
        return 0.0f;  // Invalid reading (open or short circuit)
    }
    
    // Convert ADC to voltage
    float voltage = (float)adc_value * vref / 4095.0f;
    
    // Check for invalid voltage range
    if (voltage <= 0.0f || voltage >= vref) {
        return 0.0f;
    }
    
    // Calculate NTC resistance using voltage divider formula:
    // V_adc = V_ref * R_ntc / (R_series + R_ntc)
    // Solving for R_ntc:
    // R_ntc = R_series * V_adc / (V_ref - V_adc)
    float r_ntc = r_series * voltage / (vref - voltage);
    
    return r_ntc;
}

float ntc_resistance_to_temp(float r_ntc, float r_ntc_25, float b_value) {
    if (r_ntc <= 0.0f || r_ntc_25 <= 0.0f || b_value <= 0.0f) {
        return NAN;  // Invalid parameters
    }
    
    // Steinhart-Hart equation (simplified B-parameter form):
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    // Where:
    //   T = temperature in Kelvin
    //   T0 = reference temperature (25°C = 298.15K)
    //   R = current resistance
    //   R0 = reference resistance (at 25°C)
    //   B = B-value
    
    float ln_ratio = logf(r_ntc / r_ntc_25);
    float inv_temp_k = (1.0f / NTC_T25_KELVIN) + (1.0f / b_value) * ln_ratio;
    
    if (inv_temp_k <= 0.0f) {
        return NAN;  // Invalid result
    }
    
    float temp_kelvin = 1.0f / inv_temp_k;
    float temp_celsius = temp_kelvin - 273.15f;
    
    return temp_celsius;
}

float ntc_adc_to_temp(uint16_t adc_value, float vref, float r_series, 
                      float r_ntc_25, float b_value) {
    // Convert ADC to resistance
    float r_ntc = ntc_adc_to_resistance(adc_value, vref, r_series);
    
    if (r_ntc <= 0.0f) {
        return NAN;  // Invalid reading
    }
    
    // Convert resistance to temperature
    return ntc_resistance_to_temp(r_ntc, r_ntc_25, b_value);
}

bool sensor_validate_temp(float temp_c, float min_temp, float max_temp) {
    if (isnan(temp_c) || isinf(temp_c)) {
        return false;
    }
    
    return (temp_c >= min_temp && temp_c <= max_temp);
}

