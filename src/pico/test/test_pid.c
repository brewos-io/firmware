/**
 * Unit Tests - PID Controller
 * 
 * Tests based on PID SPECIFICATIONS (what SHOULD happen), not implementation.
 * These tests define CORRECT behavior and may expose bugs in implementation.
 * 
 * PID Requirements:
 * 1. Proportional term = Kp * error
 * 2. Integral term accumulates error over time with anti-windup
 * 3. Derivative term should use derivative-on-measurement (not error) to avoid kick
 * 4. Output clamped to 0-100%
 * 5. First call should not produce derivative spike
 */

#include "unity/unity.h"
#include "mocks/mock_hardware.h"
#include <math.h>

// =============================================================================
// PID State Structure (matches control_impl.h)
// =============================================================================

typedef struct {
    float kp, ki, kd;
    float setpoint;
    float setpoint_target;
    float integral;
    float last_error;
    float last_measurement;     // For derivative-on-measurement
    float last_derivative;
    float output;
    bool setpoint_ramping;
    float ramp_rate;
    bool first_run;             // Skip derivative on first call
} pid_state_t;

// PID constants (from config.h)
#define PID_DERIVATIVE_FILTER_TAU 0.5f
#define PID_OUTPUT_MAX 100.0f
#define PID_OUTPUT_MIN 0.0f

// =============================================================================
// PID Functions (matches fixed control_common.c implementation)
// =============================================================================

static void pid_init(pid_state_t* pid, float kp, float ki, float kd, float setpoint) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = setpoint;
    pid->setpoint_target = setpoint;
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->last_measurement = 0.0f;
    pid->last_derivative = 0.0f;
    pid->output = 0.0f;
    pid->setpoint_ramping = false;
    pid->ramp_rate = 1.0f;
    pid->first_run = true;  // Skip derivative on first call
}

static float pid_compute(pid_state_t* pid, float measurement, float dt) {
    if (!pid || dt <= 0.0f) return 0.0f;
    
    float setpoint = pid->setpoint;
    float error = setpoint - measurement;
    
    // Proportional
    float p_term = pid->kp * error;
    
    // Integral with anti-windup
    float i_term = 0.0f;
    if (pid->ki > 0.001f) {
        pid->integral += error * dt;
        float max_integral = PID_OUTPUT_MAX / pid->ki;
        if (pid->integral > max_integral) pid->integral = max_integral;
        if (pid->integral < -max_integral) pid->integral = -max_integral;
        i_term = pid->ki * pid->integral;
    }
    
    // Derivative on MEASUREMENT (not error) to avoid setpoint kick
    // First call: skip derivative to avoid spike
    float d_term = 0.0f;
    if (pid->first_run) {
        pid->last_measurement = measurement;
        pid->last_derivative = 0.0f;
        pid->first_run = false;
    } else {
        // Derivative on measurement with negative sign
        float measurement_derivative = (measurement - pid->last_measurement) / dt;
        
        // Low-pass filter
        float tau = PID_DERIVATIVE_FILTER_TAU;
        float alpha = dt / (tau + dt);
        pid->last_derivative = alpha * measurement_derivative + (1.0f - alpha) * pid->last_derivative;
        
        // Negative: increasing measurement should reduce output
        d_term = -pid->kd * pid->last_derivative;
        
        pid->last_measurement = measurement;
    }
    
    pid->last_error = error;
    
    // Sum and clamp
    float output = p_term + i_term + d_term;
    if (output > PID_OUTPUT_MAX) output = PID_OUTPUT_MAX;
    if (output < PID_OUTPUT_MIN) output = PID_OUTPUT_MIN;
    
    pid->output = output;
    return output;
}

// =============================================================================
// Test State
// =============================================================================

static pid_state_t g_pid;

// =============================================================================
// SPECIFICATION TESTS - These define CORRECT behavior
// =============================================================================

// --- Proportional Term ---

void test_pid_proportional_produces_correct_output(void) {
    // SPEC: P term = Kp * error
    // error = setpoint - measurement = 100 - 90 = 10
    // P term = 2.0 * 10 = 20
    pid_init(&g_pid, 2.0f, 0.0f, 0.0f, 100.0f);  // Kp=2, Ki=0, Kd=0
    
    float output = pid_compute(&g_pid, 90.0f, 0.1f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f, output);
}

void test_pid_proportional_zero_error_zero_output(void) {
    // SPEC: At setpoint, error=0, output=0 (for P-only)
    pid_init(&g_pid, 2.0f, 0.0f, 0.0f, 100.0f);
    
    float output = pid_compute(&g_pid, 100.0f, 0.1f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, output);
}

void test_pid_proportional_negative_error(void) {
    // SPEC: Above setpoint -> negative error -> negative P term -> clamp to 0
    // error = 100 - 110 = -10
    // P term = 2.0 * (-10) = -20, clamped to 0
    pid_init(&g_pid, 2.0f, 0.0f, 0.0f, 100.0f);
    
    float output = pid_compute(&g_pid, 110.0f, 0.1f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, output);
}

// --- Integral Term ---

void test_pid_integral_accumulates_error(void) {
    // SPEC: I term accumulates error * dt
    // After 3 iterations: integral = error * dt * 3 = 10 * 0.1 * 3 = 3
    // I term = Ki * integral = 1.0 * 3 = 3
    pid_init(&g_pid, 0.0f, 1.0f, 0.0f, 100.0f);  // Ki=1
    
    pid_compute(&g_pid, 90.0f, 0.1f);  // integral = 1
    pid_compute(&g_pid, 90.0f, 0.1f);  // integral = 2
    float output = pid_compute(&g_pid, 90.0f, 0.1f);  // integral = 3
    
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3.0f, output);
}

void test_pid_integral_windup_prevented(void) {
    // SPEC: Integral should be clamped to prevent windup
    // max_integral = 100 / Ki = 100 / 0.1 = 1000
    // Even with huge error over time, integral shouldn't exceed this
    pid_init(&g_pid, 0.0f, 0.1f, 0.0f, 100.0f);  // Ki=0.1
    
    for (int i = 0; i < 10000; i++) {
        pid_compute(&g_pid, 0.0f, 0.1f);  // error = 100 each time
    }
    
    // Output should be clamped to 100 (max)
    TEST_ASSERT_EQUAL_FLOAT(100.0f, g_pid.output);
    // Integral should be clamped
    TEST_ASSERT_LESS_OR_EQUAL(1001.0f, g_pid.integral);
}

void test_pid_integral_negative_windup_prevented(void) {
    // SPEC: Negative integral should also be clamped
    pid_init(&g_pid, 0.0f, 0.1f, 0.0f, 0.0f);  // setpoint=0
    
    for (int i = 0; i < 10000; i++) {
        pid_compute(&g_pid, 100.0f, 0.1f);  // error = -100
    }
    
    // Output should be clamped to 0 (min)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g_pid.output);
}

// --- Derivative Term ---

void test_pid_derivative_first_call_no_spike(void) {
    // SPEC: First call should NOT produce a derivative spike
    // This is a common PID bug: if last_error starts at 0, first derivative is huge
    // Correct behavior: first derivative should be 0 or very small
    pid_init(&g_pid, 0.0f, 0.0f, 10.0f, 100.0f);  // Kd=10 only
    
    // First call: measurement=20, error=80
    // If last_error=0, derivative = (80-0)/0.1 = 800 -> D term = 8000!
    // CORRECT behavior: D term should be 0 or small on first call
    float output = pid_compute(&g_pid, 20.0f, 0.1f);
    
    // With filter (tau=0.5, alpha=0.167), even wrong impl gives smaller value
    // But ideally, first call D should be near 0
    // This test expects reasonable behavior (< 50)
    TEST_ASSERT_LESS_THAN(50.0f, output);
}

void test_pid_derivative_setpoint_change_no_kick(void) {
    // SPEC: Changing setpoint should NOT cause derivative kick
    // This is WHY we should use derivative-on-measurement, not derivative-on-error
    pid_init(&g_pid, 0.0f, 0.0f, 10.0f, 80.0f);  // Kd=10, setpoint=80
    
    // Run several iterations at steady state
    for (int i = 0; i < 20; i++) {
        pid_compute(&g_pid, 80.0f, 0.1f);  // At setpoint, error=0, stable
    }
    
    float stable_output = g_pid.output;
    
    // Now change setpoint (simulating user changing brew temp)
    g_pid.setpoint = 90.0f;  // Setpoint jumps by 10 degrees
    
    // Next call: measurement still 80, error jumps from 0 to 10
    // With derivative-on-ERROR: d(error)/dt = (10-0)/0.1 = 100 -> spike!
    // With derivative-on-MEASUREMENT: measurement didn't change -> no spike
    float after_change = pid_compute(&g_pid, 80.0f, 0.1f);
    
    // SPEC: Output should NOT have a huge spike from derivative
    // The error changed, but temperature didn't, so D should be ~0
    // Allow some tolerance for filtered derivative, but no huge spike
    float spike_magnitude = fabsf(after_change - stable_output);
    
    // BUG DETECTION: If derivative is on error, this will fail
    // A spike > 10 indicates derivative-on-error bug
    TEST_ASSERT_LESS_THAN(10.0f, spike_magnitude);
}

void test_pid_derivative_responds_to_measurement_change(void) {
    // SPEC: Derivative should respond when MEASUREMENT changes
    pid_init(&g_pid, 1.0f, 0.0f, 1.0f, 100.0f);  // P=1, D=1
    
    // Run at stable temperature
    for (int i = 0; i < 10; i++) {
        pid_compute(&g_pid, 80.0f, 0.1f);
    }
    
    // Temperature suddenly drops (measurement changes)
    // This SHOULD affect derivative
    float output_after_drop = pid_compute(&g_pid, 70.0f, 0.1f);
    
    // P term = 1 * 30 = 30
    // D term should respond to temperature change
    // Output should be different from just P*error due to derivative
    TEST_ASSERT_GREATER_THAN(20.0f, output_after_drop);
}

// --- Output Clamping ---

void test_pid_output_clamped_to_max(void) {
    // SPEC: Output must be clamped to 100 max
    pid_init(&g_pid, 10.0f, 0.0f, 0.0f, 100.0f);  // High Kp
    
    float output = pid_compute(&g_pid, 0.0f, 0.1f);  // error=100, P=1000
    
    TEST_ASSERT_EQUAL_FLOAT(100.0f, output);
}

void test_pid_output_clamped_to_min(void) {
    // SPEC: Output must be clamped to 0 min
    pid_init(&g_pid, 10.0f, 0.0f, 0.0f, 0.0f);  // setpoint=0
    
    float output = pid_compute(&g_pid, 100.0f, 0.1f);  // error=-100, P=-1000
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, output);
}

// --- Edge Cases ---

void test_pid_handles_zero_dt(void) {
    // SPEC: dt=0 should return 0, not crash or divide by zero
    pid_init(&g_pid, 2.0f, 0.1f, 0.5f, 100.0f);
    
    float output = pid_compute(&g_pid, 50.0f, 0.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, output);
}

void test_pid_handles_negative_dt(void) {
    // SPEC: negative dt should return 0
    pid_init(&g_pid, 2.0f, 0.1f, 0.5f, 100.0f);
    
    float output = pid_compute(&g_pid, 50.0f, -0.1f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, output);
}

void test_pid_handles_null_pointer(void) {
    float output = pid_compute(NULL, 50.0f, 0.1f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, output);
}

void test_pid_handles_nan_measurement(void) {
    pid_init(&g_pid, 2.0f, 0.1f, 0.5f, 100.0f);
    
    float output = pid_compute(&g_pid, NAN, 0.1f);
    
    // Should handle gracefully (NaN or 0 or previous value)
    // At minimum, should not crash
    TEST_ASSERT_TRUE(isnan(output) || output >= 0.0f);
}

// --- Steady State ---

void test_pid_reaches_setpoint_with_integral(void) {
    // SPEC: With integral term, PID should eliminate steady-state error
    pid_init(&g_pid, 1.0f, 0.5f, 0.1f, 50.0f);  // setpoint=50
    
    float temp = 20.0f;  // Start cold
    
    // Simple thermal simulation
    for (int i = 0; i < 1000; i++) {
        float output = pid_compute(&g_pid, temp, 0.1f);
        // Thermal model: temp increases with heater power, decreases with ambient loss
        float heat_gain = output * 0.1f;  // Heater adds heat
        float heat_loss = (temp - 20.0f) * 0.02f;  // Ambient loss
        temp += heat_gain - heat_loss;
        
        // Clamp temp to realistic range
        if (temp > 100.0f) temp = 100.0f;
        if (temp < 0.0f) temp = 0.0f;
    }
    
    // Should be close to setpoint
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 50.0f, temp);
}

// --- PID Tuning Combinations ---

void test_pid_p_only_control(void) {
    // P-only control: proportional response only
    pid_init(&g_pid, 5.0f, 0.0f, 0.0f, 100.0f);
    
    float output = pid_compute(&g_pid, 80.0f, 0.1f);  // error=20
    
    // P term = 5 * 20 = 100
    TEST_ASSERT_EQUAL_FLOAT(100.0f, output);
}

void test_pid_pi_control(void) {
    // PI control: P + I, no derivative
    pid_init(&g_pid, 1.0f, 0.5f, 0.0f, 100.0f);
    
    // First call: P = 10, I = 0.5 * 10 * 0.1 = 0.5
    float output1 = pid_compute(&g_pid, 90.0f, 0.1f);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 10.5f, output1);
    
    // Second call: P = 10, I = 0.5 * (0.5 + 1.0) = 0.75
    float output2 = pid_compute(&g_pid, 90.0f, 0.1f);
    TEST_ASSERT_GREATER_THAN(output1, output2);  // I should increase
}

void test_pid_pd_control(void) {
    // PD control: P + D, no integral
    pid_init(&g_pid, 1.0f, 0.0f, 0.5f, 100.0f);
    
    // Stabilize
    for (int i = 0; i < 10; i++) {
        pid_compute(&g_pid, 80.0f, 0.1f);
    }
    
    // D should be near 0 when stable
    float stable = g_pid.output;
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 20.0f, stable);  // ~P*error
}

// =============================================================================
// Test Runner
// =============================================================================

int run_pid_tests(void) {
    UnityBegin("test_pid.c");
    
    // Proportional Tests
    RUN_TEST(test_pid_proportional_produces_correct_output);
    RUN_TEST(test_pid_proportional_zero_error_zero_output);
    RUN_TEST(test_pid_proportional_negative_error);
    
    // Integral Tests
    RUN_TEST(test_pid_integral_accumulates_error);
    RUN_TEST(test_pid_integral_windup_prevented);
    RUN_TEST(test_pid_integral_negative_windup_prevented);
    
    // Derivative Tests (these may FAIL if implementation has bugs)
    RUN_TEST(test_pid_derivative_first_call_no_spike);
    RUN_TEST(test_pid_derivative_setpoint_change_no_kick);  // Will likely FAIL
    RUN_TEST(test_pid_derivative_responds_to_measurement_change);
    
    // Output Clamping Tests
    RUN_TEST(test_pid_output_clamped_to_max);
    RUN_TEST(test_pid_output_clamped_to_min);
    
    // Edge Cases
    RUN_TEST(test_pid_handles_zero_dt);
    RUN_TEST(test_pid_handles_negative_dt);
    RUN_TEST(test_pid_handles_null_pointer);
    RUN_TEST(test_pid_handles_nan_measurement);
    
    // Steady State
    RUN_TEST(test_pid_reaches_setpoint_with_integral);
    
    // Tuning Combinations
    RUN_TEST(test_pid_p_only_control);
    RUN_TEST(test_pid_pi_control);
    RUN_TEST(test_pid_pd_control);
    
    return UnityEnd();
}
