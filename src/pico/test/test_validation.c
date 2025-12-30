/**
 * BrewOS Pico Firmware - Validation Module Tests
 */

#include "unity/unity.h"
#include "validation.h"

// Note: setUp and tearDown are provided globally in test_main.c

// =============================================================================
// Temperature Validation Tests
// =============================================================================

void test_validate_temperature_valid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_temperature(1000, 0, 2000));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_temperature(0, 0, 2000));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_temperature(2000, 0, 2000));
}

void test_validate_temperature_out_of_range(void) {
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_temperature(-1, 0, 2000));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_temperature(2001, 0, 2000));
}

void test_validate_setpoint_target_valid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_setpoint_target(0));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_setpoint_target(1));
}

void test_validate_setpoint_target_invalid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_INVALID_TARGET, validate_setpoint_target(2));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_INVALID_TARGET, validate_setpoint_target(255));
}

// =============================================================================
// PID Validation Tests
// =============================================================================

void test_validate_pid_gains_valid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_pid_gains(100, 10, 50));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_pid_gains(0, 0, 0));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_pid_gains(10000, 10000, 10000));
}

void test_validate_pid_gains_invalid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_pid_gains(10001, 100, 100));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_pid_gains(100, 10001, 100));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_pid_gains(100, 100, 10001));
}

// =============================================================================
// Electrical Validation Tests
// =============================================================================

void test_validate_voltage_valid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_voltage(110));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_voltage(220));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_voltage(240));
}

void test_validate_voltage_invalid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_voltage(50));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_voltage(300));
}

void test_validate_current_valid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_current(10.0f));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_current(1.0f));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_current(50.0f));
}

void test_validate_current_invalid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_current(0.5f));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_current(51.0f));
}

// =============================================================================
// Buffer Validation Tests
// =============================================================================

void test_validate_buffer_copy_valid(void) {
    char dest[10];
    char src[] = "hello";
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_buffer_copy(dest, src, 6, 10));
}

void test_validate_buffer_copy_null_ptr(void) {
    char dest[10];
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_NULL_PTR, validate_buffer_copy(NULL, dest, 6, 10));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_NULL_PTR, validate_buffer_copy(dest, NULL, 6, 10));
}

void test_validate_buffer_copy_too_small(void) {
    char dest[5];
    char src[] = "hello world";
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_BUFFER_TOO_SMALL, validate_buffer_copy(dest, src, 12, 5));
}

// =============================================================================
// Pre-infusion Validation Tests
// =============================================================================

void test_validate_preinfusion_timing_valid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_preinfusion_timing(3000, 5000));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_preinfusion_timing(0, 0));
    TEST_ASSERT_EQUAL(VALIDATION_OK, validate_preinfusion_timing(10000, 30000));
}

void test_validate_preinfusion_timing_invalid(void) {
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_preinfusion_timing(10001, 5000));
    TEST_ASSERT_EQUAL(VALIDATION_ERROR_OUT_OF_RANGE, validate_preinfusion_timing(3000, 30001));
}

// =============================================================================
// Error String Tests
// =============================================================================

void test_validation_error_string(void) {
    TEST_ASSERT_NOT_NULL(validation_error_string(VALIDATION_OK));
    TEST_ASSERT_NOT_NULL(validation_error_string(VALIDATION_ERROR_NULL_PTR));
    TEST_ASSERT_NOT_NULL(validation_error_string(VALIDATION_ERROR_OUT_OF_RANGE));
}

// =============================================================================
// Test Runner
// =============================================================================

int run_validation_tests(void) {
    UnityBegin("test_validation.c");
    
    RUN_TEST(test_validate_temperature_valid);
    RUN_TEST(test_validate_temperature_out_of_range);
    RUN_TEST(test_validate_setpoint_target_valid);
    RUN_TEST(test_validate_setpoint_target_invalid);
    
    RUN_TEST(test_validate_pid_gains_valid);
    RUN_TEST(test_validate_pid_gains_invalid);
    
    RUN_TEST(test_validate_voltage_valid);
    RUN_TEST(test_validate_voltage_invalid);
    RUN_TEST(test_validate_current_valid);
    RUN_TEST(test_validate_current_invalid);
    
    RUN_TEST(test_validate_buffer_copy_valid);
    RUN_TEST(test_validate_buffer_copy_null_ptr);
    RUN_TEST(test_validate_buffer_copy_too_small);
    
    RUN_TEST(test_validate_preinfusion_timing_valid);
    RUN_TEST(test_validate_preinfusion_timing_invalid);
    
    RUN_TEST(test_validation_error_string);
    
    return UnityEnd();
}
