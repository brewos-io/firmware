/**
 * Pico Firmware - Sensor Utility Functions
 * 
 * Helper functions for sensor data processing:
 * - NTC thermistor temperature conversion (Steinhart-Hart)
 * - Moving average filtering
 * - Sensor validation
 */

#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// =============================================================================
// NTC Thermistor Configuration
// =============================================================================

// NTC 3.3kΩ @ 25°C specifications
#define NTC_R25_OHMS        3300.0f      // Resistance at 25°C
#define NTC_B_VALUE         3950.0f      // B-value (K)
#define NTC_SERIES_R_OHMS   3300.0f      // Series resistor (Ω)
#define NTC_T25_KELVIN      298.15f      // 25°C in Kelvin

// =============================================================================
// Median Filter
// =============================================================================

/**
 * Median filter structure
 * Used as first-stage filter to reject noise spikes before moving average
 */
typedef struct {
    float* buffer;          // Buffer for samples
    uint8_t size;           // Buffer size (should be odd for true median)
    uint8_t index;          // Current write index
    uint8_t count;          // Number of samples collected
} median_filter_t;

/**
 * Initialize median filter
 * 
 * @param filter Filter structure to initialize
 * @param buffer Buffer array (must be allocated externally)
 * @param size Buffer size (number of samples, should be odd)
 */
void filter_median_init(median_filter_t* filter, float* buffer, uint8_t size);

/**
 * Add sample to filter and get median value
 * 
 * @param filter Filter structure
 * @param value New sample value
 * @return Median value from buffer
 */
float filter_median_update(median_filter_t* filter, float value);

/**
 * Reset filter (clear all samples)
 * 
 * @param filter Filter structure
 */
void filter_median_reset(median_filter_t* filter);

// =============================================================================
// Moving Average Filter
// =============================================================================

/**
 * Moving average filter structure
 */
typedef struct {
    float* buffer;          // Buffer for samples
    uint8_t size;           // Buffer size
    uint8_t index;          // Current write index
    uint8_t count;          // Number of samples collected
    float sum;              // Running sum for efficiency
} moving_avg_filter_t;

/**
 * Initialize moving average filter
 * 
 * @param filter Filter structure to initialize
 * @param buffer Buffer array (must be allocated externally)
 * @param size Buffer size (number of samples)
 */
void filter_moving_avg_init(moving_avg_filter_t* filter, float* buffer, uint8_t size);

/**
 * Add sample to filter and get filtered value
 * 
 * @param filter Filter structure
 * @param value New sample value
 * @return Filtered value
 */
float filter_moving_avg_update(moving_avg_filter_t* filter, float value);

/**
 * Reset filter (clear all samples)
 * 
 * @param filter Filter structure
 */
void filter_moving_avg_reset(moving_avg_filter_t* filter);

// =============================================================================
// NTC Temperature Conversion
// =============================================================================

/**
 * Convert ADC reading to NTC resistance
 * 
 * @param adc_value ADC reading (0-4095 for 12-bit)
 * @param vref Reference voltage (typically 3.3V)
 * @param r_series Series resistor value (Ω)
 * @return NTC resistance in ohms, or 0.0 on error
 */
float ntc_adc_to_resistance(uint16_t adc_value, float vref, float r_series);

/**
 * Convert NTC resistance to temperature using Steinhart-Hart (B-parameter)
 * 
 * @param r_ntc NTC resistance in ohms
 * @param r_ntc_25 Resistance at 25°C in ohms
 * @param b_value B-value (K)
 * @return Temperature in Celsius, or NAN on error
 */
float ntc_resistance_to_temp(float r_ntc, float r_ntc_25, float b_value);

/**
 * Convert ADC reading directly to temperature
 * 
 * @param adc_value ADC reading (0-4095)
 * @param vref Reference voltage (typically 3.3V)
 * @param r_series Series resistor value (Ω)
 * @param r_ntc_25 NTC resistance at 25°C (Ω)
 * @param b_value B-value (K)
 * @return Temperature in Celsius, or NAN on error
 */
float ntc_adc_to_temp(uint16_t adc_value, float vref, float r_series, 
                      float r_ntc_25, float b_value);

/**
 * Validate temperature reading
 * 
 * @param temp_c Temperature in Celsius
 * @param min_temp Minimum valid temperature
 * @param max_temp Maximum valid temperature
 * @return true if valid, false otherwise
 */
bool sensor_validate_temp(float temp_c, float min_temp, float max_temp);

#endif // SENSOR_UTILS_H

