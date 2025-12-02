/**
 * Unit Tests - Sensor Utilities
 * 
 * Tests based on PHYSICAL SPECIFICATIONS, not implementation.
 * 
 * NTC Thermistor Specifications:
 * - 3.3kΩ @ 25°C (NTC R25)
 * - B-value: 3950K
 * - Series resistor: 3.3kΩ
 * - ADC: 12-bit (0-4095)
 * - Vref: 3.3V
 * 
 * Known temperature-resistance relationships for verification:
 * - 25°C: 3300Ω (by definition)
 * - 0°C: ~9950Ω
 * - 93°C (brew): ~340Ω
 * - 140°C (steam): ~110Ω
 */

#include "unity/unity.h"
#include "mocks/mock_hardware.h"

// Include the actual source to test real implementation
#include "../src/sensor_utils.c"

// =============================================================================
// Physical Constants for NTC 3.3k B=3950
// =============================================================================

// Calculated resistance values at key temperatures using Steinhart-Hart:
// R(T) = R25 * exp(B * (1/T - 1/T25))
// where T is in Kelvin, T25 = 298.15K
//
// CORRECT CALCULATIONS:
// R(0°C = 273.15K)  = 3300 * exp(3950 * (1/273.15 - 1/298.15)) = 11,060Ω
// R(25°C = 298.15K) = 3300Ω (by definition)
// R(93°C = 366.15K) = 3300 * exp(3950 * (1/366.15 - 1/298.15)) = 282Ω
// R(140°C = 413.15K) = 3300 * exp(3950 * (1/413.15 - 1/298.15)) = 84Ω

#define RESISTANCE_AT_0C    11060.0f  // R at 0°C
#define RESISTANCE_AT_25C   3300.0f   // R at 25°C (definition)
#define RESISTANCE_AT_93C   282.0f    // R at 93°C (brew temp)
#define RESISTANCE_AT_140C  84.0f     // R at 140°C (steam temp)

// ADC values for resistor divider: ADC = 4095 * R_ntc / (R_ntc + R_series)
#define ADC_AT_0C    3150   // 4095 * 11060 / (11060 + 3300) ≈ 3150
#define ADC_AT_25C   2048   // 4095 * 3300 / (3300 + 3300) ≈ 2048 (midpoint)
#define ADC_AT_93C   322    // 4095 * 282 / (282 + 3300) ≈ 322
#define ADC_AT_140C  102    // 4095 * 84 / (84 + 3300) ≈ 102

// =============================================================================
// NTC ADC to Resistance Tests (Voltage Divider Physics)
// =============================================================================

void test_ntc_adc_to_resistance_at_25c(void) {
    // SPEC: At 25°C, NTC resistance = 3300Ω = series resistor
    // ADC = 4095 * R_ntc / (R_ntc + R_series) = 4095 * 3300 / 6600 = 2047.5
    float resistance = ntc_adc_to_resistance(2048, 3.3f, 3300.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(50.0f, RESISTANCE_AT_25C, resistance);
}

void test_ntc_adc_to_resistance_at_0c(void) {
    // SPEC: At 0°C, resistance ≈ 11kΩ
    // High resistance = high ADC (more voltage at ADC pin)
    float resistance = ntc_adc_to_resistance(ADC_AT_0C, 3.3f, 3300.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(1000.0f, RESISTANCE_AT_0C, resistance);
}

void test_ntc_adc_to_resistance_at_93c(void) {
    // SPEC: At 93°C (brew temp), resistance ≈ 282Ω
    // Low resistance = low ADC
    float resistance = ntc_adc_to_resistance(ADC_AT_93C, 3.3f, 3300.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(50.0f, RESISTANCE_AT_93C, resistance);
}

void test_ntc_adc_to_resistance_at_140c(void) {
    // SPEC: At 140°C (steam temp), resistance ≈ 84Ω
    float resistance = ntc_adc_to_resistance(ADC_AT_140C, 3.3f, 3300.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(20.0f, RESISTANCE_AT_140C, resistance);
}

void test_ntc_adc_to_resistance_zero_returns_error(void) {
    // SPEC: ADC=0 means open circuit or shorted to ground
    float resistance = ntc_adc_to_resistance(0, 3.3f, 3300.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, resistance);  // Error condition
}

void test_ntc_adc_to_resistance_max_returns_error(void) {
    // SPEC: ADC=4095 means shorted NTC (should return error or very low R)
    float resistance = ntc_adc_to_resistance(4095, 3.3f, 3300.0f);
    
    // Could be 0 (error) or very small (essentially shorted)
    TEST_ASSERT_LESS_THAN(10.0f, resistance);
}

// =============================================================================
// NTC Resistance to Temperature Tests (Steinhart-Hart Physics)
// =============================================================================

void test_ntc_resistance_to_temp_at_25c(void) {
    // SPEC: R = 3300Ω → T = 25°C (by definition of R25)
    float temp = ntc_resistance_to_temp(RESISTANCE_AT_25C, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 25.0f, temp);
}

void test_ntc_resistance_to_temp_at_0c(void) {
    // SPEC: R ≈ 11060Ω → T = 0°C
    float temp = ntc_resistance_to_temp(RESISTANCE_AT_0C, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 0.0f, temp);
}

void test_ntc_resistance_to_temp_at_93c(void) {
    // SPEC: R ≈ 282Ω → T = 93°C (brew temperature)
    float temp = ntc_resistance_to_temp(RESISTANCE_AT_93C, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 93.0f, temp);
}

void test_ntc_resistance_to_temp_at_140c(void) {
    // SPEC: R ≈ 84Ω → T = 140°C (steam temperature)
    float temp = ntc_resistance_to_temp(RESISTANCE_AT_140C, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 140.0f, temp);
}

void test_ntc_resistance_to_temp_zero_returns_nan(void) {
    // SPEC: R=0 is physically impossible, return NaN
    float temp = ntc_resistance_to_temp(0.0f, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_IS_NAN(temp);
}

void test_ntc_resistance_to_temp_negative_returns_nan(void) {
    // SPEC: Negative resistance is physically impossible
    float temp = ntc_resistance_to_temp(-100.0f, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_IS_NAN(temp);
}

// =============================================================================
// NTC ADC to Temperature (Full Pipeline) Tests
// =============================================================================

void test_ntc_adc_to_temp_room_temperature(void) {
    // SPEC: ADC ≈ 2048 → 25°C
    float temp = ntc_adc_to_temp(2048, 3.3f, 3300.0f, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 25.0f, temp);
}

void test_ntc_adc_to_temp_brew_temperature(void) {
    // SPEC: ADC ≈ 322 → 93°C (brew temp)
    float temp = ntc_adc_to_temp(ADC_AT_93C, 3.3f, 3300.0f, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(3.0f, 93.0f, temp);
}

void test_ntc_adc_to_temp_steam_temperature(void) {
    // SPEC: ADC ≈ 102 → 140°C (steam temp)
    float temp = ntc_adc_to_temp(ADC_AT_140C, 3.3f, 3300.0f, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 140.0f, temp);
}

void test_ntc_adc_to_temp_cold_temperature(void) {
    // SPEC: ADC ≈ 3150 → 0°C
    float temp = ntc_adc_to_temp(ADC_AT_0C, 3.3f, 3300.0f, 3300.0f, 3950.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(3.0f, 0.0f, temp);
}

// =============================================================================
// Temperature Validation Tests
// =============================================================================

void test_sensor_validate_temp_valid_brew_range(void) {
    // SPEC: Valid brew temperatures are 85-100°C
    TEST_ASSERT_TRUE(sensor_validate_temp(93.0f, 85.0f, 100.0f));
    TEST_ASSERT_TRUE(sensor_validate_temp(85.0f, 85.0f, 100.0f));
    TEST_ASSERT_TRUE(sensor_validate_temp(100.0f, 85.0f, 100.0f));
}

void test_sensor_validate_temp_invalid_brew_range(void) {
    TEST_ASSERT_FALSE(sensor_validate_temp(84.9f, 85.0f, 100.0f));
    TEST_ASSERT_FALSE(sensor_validate_temp(100.1f, 85.0f, 100.0f));
}

void test_sensor_validate_temp_nan_invalid(void) {
    // SPEC: NaN is always invalid
    TEST_ASSERT_FALSE(sensor_validate_temp(NAN, -50.0f, 200.0f));
}

void test_sensor_validate_temp_inf_invalid(void) {
    // SPEC: Infinity is always invalid
    TEST_ASSERT_FALSE(sensor_validate_temp(INFINITY, -50.0f, 200.0f));
    TEST_ASSERT_FALSE(sensor_validate_temp(-INFINITY, -50.0f, 200.0f));
}

// =============================================================================
// Moving Average Filter Tests (Mathematical Correctness)
// =============================================================================

void test_filter_moving_avg_init_correct(void) {
    moving_avg_filter_t filter;
    float buffer[5];
    
    filter_moving_avg_init(&filter, buffer, 5);
    
    TEST_ASSERT_EQUAL_UINT8(5, filter.size);
    TEST_ASSERT_EQUAL_UINT8(0, filter.index);
    TEST_ASSERT_EQUAL_UINT8(0, filter.count);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, filter.sum);
}

void test_filter_moving_avg_first_sample_returns_sample(void) {
    // SPEC: First sample returns itself (average of 1 sample)
    moving_avg_filter_t filter;
    float buffer[5];
    
    filter_moving_avg_init(&filter, buffer, 5);
    float result = filter_moving_avg_update(&filter, 42.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(42.0f, result);
}

void test_filter_moving_avg_correct_average(void) {
    // SPEC: Average of {10, 20, 30} = 20
    moving_avg_filter_t filter;
    float buffer[5];
    
    filter_moving_avg_init(&filter, buffer, 5);
    
    filter_moving_avg_update(&filter, 10.0f);
    filter_moving_avg_update(&filter, 20.0f);
    float result = filter_moving_avg_update(&filter, 30.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(20.0f, result);
}

void test_filter_moving_avg_full_buffer_correct(void) {
    // SPEC: Average of {10, 20, 30, 40, 50} = 30
    moving_avg_filter_t filter;
    float buffer[5];
    
    filter_moving_avg_init(&filter, buffer, 5);
    
    filter_moving_avg_update(&filter, 10.0f);
    filter_moving_avg_update(&filter, 20.0f);
    filter_moving_avg_update(&filter, 30.0f);
    filter_moving_avg_update(&filter, 40.0f);
    float result = filter_moving_avg_update(&filter, 50.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(30.0f, result);
}

void test_filter_moving_avg_sliding_window(void) {
    // SPEC: When buffer is full, oldest value is replaced
    // Buffer: {10, 20, 30} -> add 100 -> {100, 20, 30} -> avg = 50
    moving_avg_filter_t filter;
    float buffer[3];
    
    filter_moving_avg_init(&filter, buffer, 3);
    
    filter_moving_avg_update(&filter, 10.0f);  // [10, _, _] avg=10
    filter_moving_avg_update(&filter, 20.0f);  // [10, 20, _] avg=15
    filter_moving_avg_update(&filter, 30.0f);  // [10, 20, 30] avg=20
    float result = filter_moving_avg_update(&filter, 100.0f);  // [100, 20, 30] avg=50
    
    TEST_ASSERT_EQUAL_FLOAT(50.0f, result);
}

void test_filter_moving_avg_reset_works(void) {
    moving_avg_filter_t filter;
    float buffer[5];
    
    filter_moving_avg_init(&filter, buffer, 5);
    
    filter_moving_avg_update(&filter, 100.0f);
    filter_moving_avg_update(&filter, 200.0f);
    
    filter_moving_avg_reset(&filter);
    
    TEST_ASSERT_EQUAL_UINT8(0, filter.count);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, filter.sum);
    
    // After reset, next sample returns itself
    float result = filter_moving_avg_update(&filter, 50.0f);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, result);
}

void test_filter_moving_avg_constant_input_stable(void) {
    // SPEC: Constant input gives constant output
    moving_avg_filter_t filter;
    float buffer[10];
    
    filter_moving_avg_init(&filter, buffer, 10);
    
    for (int i = 0; i < 50; i++) {
        float result = filter_moving_avg_update(&filter, 42.0f);
        TEST_ASSERT_EQUAL_FLOAT(42.0f, result);
    }
}

void test_filter_moving_avg_null_safety(void) {
    float buffer[5];
    moving_avg_filter_t filter;
    
    // Should not crash with NULL
    filter_moving_avg_init(NULL, buffer, 5);
    filter_moving_avg_init(&filter, NULL, 5);
    filter_moving_avg_reset(NULL);
    
    // Invalid filter should return input unchanged
    filter.buffer = NULL;
    float result = filter_moving_avg_update(&filter, 99.0f);
    TEST_ASSERT_EQUAL_FLOAT(99.0f, result);
}

// =============================================================================
// Test Runner
// =============================================================================

int run_sensor_utils_tests(void) {
    UnityBegin("test_sensor_utils.c");
    
    // NTC ADC to Resistance
    RUN_TEST(test_ntc_adc_to_resistance_at_25c);
    RUN_TEST(test_ntc_adc_to_resistance_at_0c);
    RUN_TEST(test_ntc_adc_to_resistance_at_93c);
    RUN_TEST(test_ntc_adc_to_resistance_at_140c);
    RUN_TEST(test_ntc_adc_to_resistance_zero_returns_error);
    RUN_TEST(test_ntc_adc_to_resistance_max_returns_error);
    
    // NTC Resistance to Temperature
    RUN_TEST(test_ntc_resistance_to_temp_at_25c);
    RUN_TEST(test_ntc_resistance_to_temp_at_0c);
    RUN_TEST(test_ntc_resistance_to_temp_at_93c);
    RUN_TEST(test_ntc_resistance_to_temp_at_140c);
    RUN_TEST(test_ntc_resistance_to_temp_zero_returns_nan);
    RUN_TEST(test_ntc_resistance_to_temp_negative_returns_nan);
    
    // NTC ADC to Temperature (Full Pipeline)
    RUN_TEST(test_ntc_adc_to_temp_room_temperature);
    RUN_TEST(test_ntc_adc_to_temp_brew_temperature);
    RUN_TEST(test_ntc_adc_to_temp_steam_temperature);
    RUN_TEST(test_ntc_adc_to_temp_cold_temperature);
    
    // Temperature Validation
    RUN_TEST(test_sensor_validate_temp_valid_brew_range);
    RUN_TEST(test_sensor_validate_temp_invalid_brew_range);
    RUN_TEST(test_sensor_validate_temp_nan_invalid);
    RUN_TEST(test_sensor_validate_temp_inf_invalid);
    
    // Moving Average Filter
    RUN_TEST(test_filter_moving_avg_init_correct);
    RUN_TEST(test_filter_moving_avg_first_sample_returns_sample);
    RUN_TEST(test_filter_moving_avg_correct_average);
    RUN_TEST(test_filter_moving_avg_full_buffer_correct);
    RUN_TEST(test_filter_moving_avg_sliding_window);
    RUN_TEST(test_filter_moving_avg_reset_works);
    RUN_TEST(test_filter_moving_avg_constant_input_stable);
    RUN_TEST(test_filter_moving_avg_null_safety);
    
    return UnityEnd();
}
