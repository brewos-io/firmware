/**
 * Unit Tests - Configuration Validation
 * 
 * Tests for:
 * - Environmental config validation (voltage, current)
 * - Temperature setpoint validation
 * - PID gains validation
 * - Heating strategy validation
 */

#include "unity/unity.h"
#include "mocks/mock_hardware.h"

// =============================================================================
// Configuration Constants
// =============================================================================

#define MIN_VOLTAGE 100
#define MAX_VOLTAGE 250
#define MIN_CURRENT 1.0f
#define MAX_CURRENT 50.0f

#define MIN_TEMP_SETPOINT 0      // 0°C
#define MAX_TEMP_SETPOINT 2000   // 200°C (in 0.1°C units)

#define MIN_PID_GAIN 0
#define MAX_PID_GAIN 10000  // 100.0 in scaled units

// Heating strategies
#define HEAT_BREW_ONLY      0
#define HEAT_SEQUENTIAL     1
#define HEAT_PARALLEL       2
#define HEAT_SMART_STAGGER  3

// =============================================================================
// Validation Functions (simulated)
// =============================================================================

typedef struct {
    uint16_t nominal_voltage;
    float max_current_draw;
} environmental_config_t;

static bool validate_environmental_config(const environmental_config_t* config) {
    if (!config) return false;
    
    // Validate voltage
    if (config->nominal_voltage < MIN_VOLTAGE || config->nominal_voltage > MAX_VOLTAGE) {
        return false;
    }
    
    // Validate current
    if (config->max_current_draw < MIN_CURRENT || config->max_current_draw > MAX_CURRENT) {
        return false;
    }
    
    return true;
}

static bool validate_temperature_setpoint(int16_t temp) {
    return (temp >= MIN_TEMP_SETPOINT && temp <= MAX_TEMP_SETPOINT);
}

static bool validate_pid_gains(uint16_t kp, uint16_t ki, uint16_t kd) {
    // All gains must be within range
    if (kp > MAX_PID_GAIN) return false;
    if (ki > MAX_PID_GAIN) return false;
    if (kd > MAX_PID_GAIN) return false;
    return true;
}

static bool validate_heating_strategy(uint8_t strategy, float max_current, 
                                       float brew_current, float steam_current) {
    // Brew only is always allowed
    if (strategy == HEAT_BREW_ONLY) return true;
    
    // Check if combined heating is safe
    float combined_current = brew_current + steam_current;
    
    switch (strategy) {
        case HEAT_SEQUENTIAL:
            // Sequential: only one heater at a time
            return true;
            
        case HEAT_PARALLEL:
        case HEAT_SMART_STAGGER:
            // These require both heaters to run together
            // Check if current limit allows it
            return (combined_current <= max_current * 0.95f);  // 5% safety margin
            
        default:
            return false;  // Unknown strategy
    }
}

// Note: setUp and tearDown are provided globally in test_main.c

// =============================================================================
// Environmental Config Validation Tests
// =============================================================================

void test_env_config_valid_230v_16a(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = 16.0f
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_valid_120v_12a(void) {
    environmental_config_t config = {
        .nominal_voltage = 120,
        .max_current_draw = 12.0f
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_valid_240v_10a(void) {
    environmental_config_t config = {
        .nominal_voltage = 240,
        .max_current_draw = 10.0f
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_min_voltage(void) {
    environmental_config_t config = {
        .nominal_voltage = MIN_VOLTAGE,
        .max_current_draw = 10.0f
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_max_voltage(void) {
    environmental_config_t config = {
        .nominal_voltage = MAX_VOLTAGE,
        .max_current_draw = 10.0f
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_voltage_too_low(void) {
    environmental_config_t config = {
        .nominal_voltage = MIN_VOLTAGE - 1,
        .max_current_draw = 10.0f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_voltage_too_high(void) {
    environmental_config_t config = {
        .nominal_voltage = MAX_VOLTAGE + 1,
        .max_current_draw = 10.0f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_voltage_zero(void) {
    environmental_config_t config = {
        .nominal_voltage = 0,
        .max_current_draw = 10.0f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_min_current(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = MIN_CURRENT
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_max_current(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = MAX_CURRENT
    };
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
}

void test_env_config_current_too_low(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = MIN_CURRENT - 0.1f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_current_too_high(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = MAX_CURRENT + 0.1f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_current_zero(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = 0.0f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_current_negative(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = -10.0f
    };
    TEST_ASSERT_FALSE(validate_environmental_config(&config));
}

void test_env_config_null_pointer(void) {
    TEST_ASSERT_FALSE(validate_environmental_config(NULL));
}

// =============================================================================
// Temperature Setpoint Validation Tests
// =============================================================================

void test_temp_setpoint_valid_brew(void) {
    TEST_ASSERT_TRUE(validate_temperature_setpoint(930));  // 93.0°C
}

void test_temp_setpoint_valid_steam(void) {
    TEST_ASSERT_TRUE(validate_temperature_setpoint(1400));  // 140.0°C
}

void test_temp_setpoint_minimum(void) {
    TEST_ASSERT_TRUE(validate_temperature_setpoint(MIN_TEMP_SETPOINT));
}

void test_temp_setpoint_maximum(void) {
    TEST_ASSERT_TRUE(validate_temperature_setpoint(MAX_TEMP_SETPOINT));
}

void test_temp_setpoint_negative(void) {
    TEST_ASSERT_FALSE(validate_temperature_setpoint(-1));
}

void test_temp_setpoint_too_high(void) {
    TEST_ASSERT_FALSE(validate_temperature_setpoint(MAX_TEMP_SETPOINT + 1));
}

void test_temp_setpoint_room_temp(void) {
    TEST_ASSERT_TRUE(validate_temperature_setpoint(250));  // 25.0°C
}

void test_temp_setpoint_freezing(void) {
    TEST_ASSERT_TRUE(validate_temperature_setpoint(0));  // 0.0°C
}

// =============================================================================
// PID Gains Validation Tests
// =============================================================================

void test_pid_gains_valid_typical(void) {
    // Typical values: Kp=2.0, Ki=0.1, Kd=0.5 (scaled by 100)
    TEST_ASSERT_TRUE(validate_pid_gains(200, 10, 50));
}

void test_pid_gains_valid_aggressive(void) {
    TEST_ASSERT_TRUE(validate_pid_gains(500, 100, 200));
}

void test_pid_gains_all_zero(void) {
    TEST_ASSERT_TRUE(validate_pid_gains(0, 0, 0));
}

void test_pid_gains_maximum(void) {
    TEST_ASSERT_TRUE(validate_pid_gains(MAX_PID_GAIN, MAX_PID_GAIN, MAX_PID_GAIN));
}

void test_pid_gains_kp_too_high(void) {
    TEST_ASSERT_FALSE(validate_pid_gains(MAX_PID_GAIN + 1, 10, 50));
}

void test_pid_gains_ki_too_high(void) {
    TEST_ASSERT_FALSE(validate_pid_gains(200, MAX_PID_GAIN + 1, 50));
}

void test_pid_gains_kd_too_high(void) {
    TEST_ASSERT_FALSE(validate_pid_gains(200, 10, MAX_PID_GAIN + 1));
}

void test_pid_gains_all_too_high(void) {
    TEST_ASSERT_FALSE(validate_pid_gains(MAX_PID_GAIN + 1, MAX_PID_GAIN + 1, MAX_PID_GAIN + 1));
}

// =============================================================================
// Heating Strategy Validation Tests
// =============================================================================

void test_heating_strategy_brew_only_always_allowed(void) {
    // Brew only should always be allowed regardless of current limits
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_BREW_ONLY, 5.0f, 10.0f, 10.0f));
}

void test_heating_strategy_sequential_always_allowed(void) {
    // Sequential only runs one heater at a time
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_SEQUENTIAL, 10.0f, 6.0f, 6.0f));
}

void test_heating_strategy_parallel_allowed_high_current(void) {
    // Combined current = 6 + 6 = 12A, max = 16A, margin = 15.2A
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_PARALLEL, 16.0f, 6.0f, 6.0f));
}

void test_heating_strategy_parallel_not_allowed_low_current(void) {
    // Combined current = 6 + 6 = 12A, max = 10A, margin = 9.5A
    TEST_ASSERT_FALSE(validate_heating_strategy(HEAT_PARALLEL, 10.0f, 6.0f, 6.0f));
}

void test_heating_strategy_smart_stagger_allowed(void) {
    // Smart stagger has same requirements as parallel
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_SMART_STAGGER, 16.0f, 6.0f, 6.0f));
}

void test_heating_strategy_smart_stagger_not_allowed(void) {
    TEST_ASSERT_FALSE(validate_heating_strategy(HEAT_SMART_STAGGER, 10.0f, 6.0f, 6.0f));
}

void test_heating_strategy_unknown(void) {
    TEST_ASSERT_FALSE(validate_heating_strategy(99, 16.0f, 6.0f, 6.0f));
}

void test_heating_strategy_edge_case_exact_limit(void) {
    // Combined = 9.5A exactly at margin (10 * 0.95)
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_PARALLEL, 10.0f, 4.75f, 4.75f));
}

void test_heating_strategy_edge_case_just_over_limit(void) {
    // Combined = 9.6A, just over margin (10 * 0.95 = 9.5)
    TEST_ASSERT_FALSE(validate_heating_strategy(HEAT_PARALLEL, 10.0f, 4.8f, 4.8f));
}

// =============================================================================
// Combined Validation Tests
// =============================================================================

void test_typical_israel_setup(void) {
    environmental_config_t config = {
        .nominal_voltage = 230,
        .max_current_draw = 16.0f
    };
    
    // Typical ECM Synchronika: 1200W brew, 1400W steam
    float brew_current = 1200.0f / 230.0f;   // ~5.2A
    float steam_current = 1400.0f / 230.0f;  // ~6.1A
    
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_PARALLEL, 16.0f, brew_current, steam_current));
}

void test_typical_us_setup_restrictive(void) {
    environmental_config_t config = {
        .nominal_voltage = 120,
        .max_current_draw = 12.0f  // 15A breaker with 80% rule
    };
    
    // US version: lower power heaters
    float brew_current = 1000.0f / 120.0f;   // ~8.3A
    float steam_current = 1200.0f / 120.0f;  // ~10A
    
    TEST_ASSERT_TRUE(validate_environmental_config(&config));
    // Parallel NOT allowed (combined > 12A)
    TEST_ASSERT_FALSE(validate_heating_strategy(HEAT_PARALLEL, 12.0f, brew_current, steam_current));
    // Sequential IS allowed
    TEST_ASSERT_TRUE(validate_heating_strategy(HEAT_SEQUENTIAL, 12.0f, brew_current, steam_current));
}

// =============================================================================
// Test Runner
// =============================================================================

int run_config_validation_tests(void) {
    UnityBegin("test_config_validation.c");
    
    // Environmental Config Tests
    RUN_TEST(test_env_config_valid_230v_16a);
    RUN_TEST(test_env_config_valid_120v_12a);
    RUN_TEST(test_env_config_valid_240v_10a);
    RUN_TEST(test_env_config_min_voltage);
    RUN_TEST(test_env_config_max_voltage);
    RUN_TEST(test_env_config_voltage_too_low);
    RUN_TEST(test_env_config_voltage_too_high);
    RUN_TEST(test_env_config_voltage_zero);
    RUN_TEST(test_env_config_min_current);
    RUN_TEST(test_env_config_max_current);
    RUN_TEST(test_env_config_current_too_low);
    RUN_TEST(test_env_config_current_too_high);
    RUN_TEST(test_env_config_current_zero);
    RUN_TEST(test_env_config_current_negative);
    RUN_TEST(test_env_config_null_pointer);
    
    // Temperature Setpoint Tests
    RUN_TEST(test_temp_setpoint_valid_brew);
    RUN_TEST(test_temp_setpoint_valid_steam);
    RUN_TEST(test_temp_setpoint_minimum);
    RUN_TEST(test_temp_setpoint_maximum);
    RUN_TEST(test_temp_setpoint_negative);
    RUN_TEST(test_temp_setpoint_too_high);
    RUN_TEST(test_temp_setpoint_room_temp);
    RUN_TEST(test_temp_setpoint_freezing);
    
    // PID Gains Tests
    RUN_TEST(test_pid_gains_valid_typical);
    RUN_TEST(test_pid_gains_valid_aggressive);
    RUN_TEST(test_pid_gains_all_zero);
    RUN_TEST(test_pid_gains_maximum);
    RUN_TEST(test_pid_gains_kp_too_high);
    RUN_TEST(test_pid_gains_ki_too_high);
    RUN_TEST(test_pid_gains_kd_too_high);
    RUN_TEST(test_pid_gains_all_too_high);
    
    // Heating Strategy Tests
    RUN_TEST(test_heating_strategy_brew_only_always_allowed);
    RUN_TEST(test_heating_strategy_sequential_always_allowed);
    RUN_TEST(test_heating_strategy_parallel_allowed_high_current);
    RUN_TEST(test_heating_strategy_parallel_not_allowed_low_current);
    RUN_TEST(test_heating_strategy_smart_stagger_allowed);
    RUN_TEST(test_heating_strategy_smart_stagger_not_allowed);
    RUN_TEST(test_heating_strategy_unknown);
    RUN_TEST(test_heating_strategy_edge_case_exact_limit);
    RUN_TEST(test_heating_strategy_edge_case_just_over_limit);
    
    // Combined Validation Tests
    RUN_TEST(test_typical_israel_setup);
    RUN_TEST(test_typical_us_setup_restrictive);
    
    return UnityEnd();
}

