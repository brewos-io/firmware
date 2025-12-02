/**
 * Unit Tests - Cleaning Mode
 * 
 * Tests for:
 * - Brew cycle counting
 * - Cleaning threshold management
 * - Reminder logic
 * - Flash wear reduction
 */

#include "unity/unity.h"
#include "mocks/mock_hardware.h"

// =============================================================================
// Cleaning Mode Constants (from cleaning.h)
// =============================================================================

#define CLEANING_DEFAULT_THRESHOLD    100
#define CLEANING_MIN_THRESHOLD        10
#define CLEANING_MAX_THRESHOLD        1000
#define CLEANING_CYCLE_MIN_TIME_MS    15000  // 15 seconds

// =============================================================================
// Cleaning State (simulated)
// =============================================================================

typedef struct {
    uint16_t brew_count;
    uint16_t threshold;
    bool unsaved_changes;
} cleaning_state_t;

static cleaning_state_t g_cleaning_state;

// =============================================================================
// Cleaning Functions (simulated from cleaning.c)
// =============================================================================

static void cleaning_init_test(void) {
    g_cleaning_state.brew_count = 0;
    g_cleaning_state.threshold = CLEANING_DEFAULT_THRESHOLD;
    g_cleaning_state.unsaved_changes = false;
}

static uint16_t cleaning_get_brew_count(void) {
    return g_cleaning_state.brew_count;
}

static void cleaning_reset_brew_count(void) {
    g_cleaning_state.brew_count = 0;
    g_cleaning_state.unsaved_changes = true;
}

static uint16_t cleaning_get_threshold(void) {
    return g_cleaning_state.threshold;
}

static bool cleaning_set_threshold(uint16_t threshold) {
    if (threshold < CLEANING_MIN_THRESHOLD || threshold > CLEANING_MAX_THRESHOLD) {
        return false;
    }
    g_cleaning_state.threshold = threshold;
    g_cleaning_state.unsaved_changes = true;
    return true;
}

static bool cleaning_is_reminder_due(void) {
    return g_cleaning_state.brew_count >= g_cleaning_state.threshold;
}

static void cleaning_record_brew_cycle(uint32_t brew_duration_ms) {
    // Only count brews >= 15 seconds (filters out accidental button presses)
    if (brew_duration_ms >= CLEANING_CYCLE_MIN_TIME_MS) {
        // SPEC: Saturate at UINT16_MAX to prevent overflow/data loss
        if (g_cleaning_state.brew_count < 65535) {
            g_cleaning_state.brew_count++;
        }
        g_cleaning_state.unsaved_changes = true;
    }
}

static bool cleaning_has_unsaved_changes(void) {
    return g_cleaning_state.unsaved_changes;
}

static bool cleaning_force_save(void) {
    g_cleaning_state.unsaved_changes = false;
    return true;
}

// Helper to initialize cleaning state before each test
static void test_cleaning_init(void) {
    cleaning_init_test();
}

// Note: setUp and tearDown are provided globally in test_main.c

// =============================================================================
// Initialization Tests
// =============================================================================

void test_cleaning_init_defaults(void) {
    test_cleaning_init();
    TEST_ASSERT_EQUAL_UINT16(0, cleaning_get_brew_count());
    TEST_ASSERT_EQUAL_UINT16(CLEANING_DEFAULT_THRESHOLD, cleaning_get_threshold());
    TEST_ASSERT_FALSE(cleaning_is_reminder_due());
}

// =============================================================================
// Brew Count Tests
// =============================================================================

void test_cleaning_count_valid_brew(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(20000);  // 20 seconds
    TEST_ASSERT_EQUAL_UINT16(1, cleaning_get_brew_count());
}

void test_cleaning_count_minimum_brew(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(15000);  // Exactly 15 seconds
    TEST_ASSERT_EQUAL_UINT16(1, cleaning_get_brew_count());
}

void test_cleaning_ignore_short_brew(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(14999);  // Just under 15 seconds
    TEST_ASSERT_EQUAL_UINT16(0, cleaning_get_brew_count());
}

void test_cleaning_ignore_very_short_brew(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(1000);  // 1 second
    TEST_ASSERT_EQUAL_UINT16(0, cleaning_get_brew_count());
}

void test_cleaning_ignore_zero_brew(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(0);  // Accidental press
    TEST_ASSERT_EQUAL_UINT16(0, cleaning_get_brew_count());
}

void test_cleaning_count_multiple_brews(void) {
    test_cleaning_init();
    for (int i = 0; i < 10; i++) {
        cleaning_record_brew_cycle(25000);  // 25 seconds each
    }
    TEST_ASSERT_EQUAL_UINT16(10, cleaning_get_brew_count());
}

void test_cleaning_count_mixed_brews(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(25000);  // Valid
    cleaning_record_brew_cycle(5000);   // Too short
    cleaning_record_brew_cycle(30000);  // Valid
    cleaning_record_brew_cycle(10000);  // Too short
    cleaning_record_brew_cycle(15000);  // Valid (exactly minimum)
    
    TEST_ASSERT_EQUAL_UINT16(3, cleaning_get_brew_count());
}

void test_cleaning_reset_brew_count(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(20000);
    cleaning_record_brew_cycle(20000);
    cleaning_record_brew_cycle(20000);
    TEST_ASSERT_EQUAL_UINT16(3, cleaning_get_brew_count());
    
    cleaning_reset_brew_count();
    TEST_ASSERT_EQUAL_UINT16(0, cleaning_get_brew_count());
}

// =============================================================================
// Threshold Tests
// =============================================================================

void test_cleaning_set_threshold_valid(void) {
    test_cleaning_init();
    TEST_ASSERT_TRUE(cleaning_set_threshold(50));
    TEST_ASSERT_EQUAL_UINT16(50, cleaning_get_threshold());
}

void test_cleaning_set_threshold_minimum(void) {
    test_cleaning_init();
    TEST_ASSERT_TRUE(cleaning_set_threshold(CLEANING_MIN_THRESHOLD));
    TEST_ASSERT_EQUAL_UINT16(CLEANING_MIN_THRESHOLD, cleaning_get_threshold());
}

void test_cleaning_set_threshold_maximum(void) {
    test_cleaning_init();
    TEST_ASSERT_TRUE(cleaning_set_threshold(CLEANING_MAX_THRESHOLD));
    TEST_ASSERT_EQUAL_UINT16(CLEANING_MAX_THRESHOLD, cleaning_get_threshold());
}

void test_cleaning_set_threshold_below_minimum(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_set_threshold(CLEANING_MIN_THRESHOLD - 1));
    // Threshold should remain at default
    TEST_ASSERT_EQUAL_UINT16(CLEANING_DEFAULT_THRESHOLD, cleaning_get_threshold());
}

void test_cleaning_set_threshold_above_maximum(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_set_threshold(CLEANING_MAX_THRESHOLD + 1));
    TEST_ASSERT_EQUAL_UINT16(CLEANING_DEFAULT_THRESHOLD, cleaning_get_threshold());
}

void test_cleaning_set_threshold_zero(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_set_threshold(0));
    TEST_ASSERT_EQUAL_UINT16(CLEANING_DEFAULT_THRESHOLD, cleaning_get_threshold());
}

// =============================================================================
// Reminder Tests
// =============================================================================

void test_cleaning_reminder_not_due_initially(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_is_reminder_due());
}

void test_cleaning_reminder_not_due_below_threshold(void) {
    test_cleaning_init();
    cleaning_set_threshold(100);
    
    for (int i = 0; i < 99; i++) {
        cleaning_record_brew_cycle(20000);
    }
    
    TEST_ASSERT_FALSE(cleaning_is_reminder_due());
    TEST_ASSERT_EQUAL_UINT16(99, cleaning_get_brew_count());
}

void test_cleaning_reminder_due_at_threshold(void) {
    test_cleaning_init();
    cleaning_set_threshold(100);
    
    for (int i = 0; i < 100; i++) {
        cleaning_record_brew_cycle(20000);
    }
    
    TEST_ASSERT_TRUE(cleaning_is_reminder_due());
    TEST_ASSERT_EQUAL_UINT16(100, cleaning_get_brew_count());
}

void test_cleaning_reminder_due_above_threshold(void) {
    test_cleaning_init();
    cleaning_set_threshold(100);
    
    for (int i = 0; i < 150; i++) {
        cleaning_record_brew_cycle(20000);
    }
    
    TEST_ASSERT_TRUE(cleaning_is_reminder_due());
}

void test_cleaning_reminder_clears_after_reset(void) {
    test_cleaning_init();
    cleaning_set_threshold(10);
    
    for (int i = 0; i < 10; i++) {
        cleaning_record_brew_cycle(20000);
    }
    
    TEST_ASSERT_TRUE(cleaning_is_reminder_due());
    
    cleaning_reset_brew_count();
    
    TEST_ASSERT_FALSE(cleaning_is_reminder_due());
}

void test_cleaning_reminder_with_different_thresholds(void) {
    test_cleaning_init();
    // Test with low threshold
    cleaning_set_threshold(10);
    for (int i = 0; i < 10; i++) {
        cleaning_record_brew_cycle(20000);
    }
    TEST_ASSERT_TRUE(cleaning_is_reminder_due());
    
    // Reset and test with higher threshold
    cleaning_reset_brew_count();
    cleaning_set_threshold(200);
    
    for (int i = 0; i < 199; i++) {
        cleaning_record_brew_cycle(20000);
    }
    TEST_ASSERT_FALSE(cleaning_is_reminder_due());
    
    cleaning_record_brew_cycle(20000);
    TEST_ASSERT_TRUE(cleaning_is_reminder_due());
}

// =============================================================================
// Flash Wear Reduction Tests
// =============================================================================

void test_cleaning_unsaved_changes_after_brew(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_has_unsaved_changes());
    
    cleaning_record_brew_cycle(20000);
    
    TEST_ASSERT_TRUE(cleaning_has_unsaved_changes());
}

void test_cleaning_unsaved_changes_after_threshold_change(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_has_unsaved_changes());
    
    cleaning_set_threshold(50);
    
    TEST_ASSERT_TRUE(cleaning_has_unsaved_changes());
}

void test_cleaning_unsaved_changes_after_reset(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(20000);
    cleaning_force_save();
    TEST_ASSERT_FALSE(cleaning_has_unsaved_changes());
    
    cleaning_reset_brew_count();
    
    TEST_ASSERT_TRUE(cleaning_has_unsaved_changes());
}

void test_cleaning_force_save_clears_flag(void) {
    test_cleaning_init();
    cleaning_record_brew_cycle(20000);
    TEST_ASSERT_TRUE(cleaning_has_unsaved_changes());
    
    cleaning_force_save();
    
    TEST_ASSERT_FALSE(cleaning_has_unsaved_changes());
}

void test_cleaning_short_brew_no_unsaved_changes(void) {
    test_cleaning_init();
    TEST_ASSERT_FALSE(cleaning_has_unsaved_changes());
    
    cleaning_record_brew_cycle(5000);  // Too short
    
    // Should not mark as having changes since brew wasn't counted
    TEST_ASSERT_FALSE(cleaning_has_unsaved_changes());
}

// =============================================================================
// Edge Cases
// =============================================================================

void test_cleaning_uint16_overflow_protection(void) {
    test_cleaning_init();
    // Set count to max value
    g_cleaning_state.brew_count = 65535;
    
    uint16_t before = cleaning_get_brew_count();
    TEST_ASSERT_EQUAL_UINT16(65535, before);
    
    // Record another brew - should SATURATE at 65535, NOT overflow to 0
    cleaning_record_brew_cycle(20000);
    uint16_t after = cleaning_get_brew_count();
    
    // SPEC: Count should saturate at UINT16_MAX (65535) to prevent data loss
    // Rolling over to 0 would lose the brew history, which is unacceptable
    TEST_ASSERT_EQUAL_UINT16(65535, after);
}

void test_cleaning_long_brew_duration(void) {
    test_cleaning_init();
    // Very long brew (10 minutes)
    cleaning_record_brew_cycle(600000);
    TEST_ASSERT_EQUAL_UINT16(1, cleaning_get_brew_count());
}

void test_cleaning_max_uint32_duration(void) {
    test_cleaning_init();
    // Max uint32 duration (should still count)
    cleaning_record_brew_cycle(0xFFFFFFFF);
    TEST_ASSERT_EQUAL_UINT16(1, cleaning_get_brew_count());
}

// =============================================================================
// Test Runner
// =============================================================================

int run_cleaning_tests(void) {
    UnityBegin("test_cleaning.c");
    
    // Initialization Tests
    RUN_TEST(test_cleaning_init_defaults);
    
    // Brew Count Tests
    RUN_TEST(test_cleaning_count_valid_brew);
    RUN_TEST(test_cleaning_count_minimum_brew);
    RUN_TEST(test_cleaning_ignore_short_brew);
    RUN_TEST(test_cleaning_ignore_very_short_brew);
    RUN_TEST(test_cleaning_ignore_zero_brew);
    RUN_TEST(test_cleaning_count_multiple_brews);
    RUN_TEST(test_cleaning_count_mixed_brews);
    RUN_TEST(test_cleaning_reset_brew_count);
    
    // Threshold Tests
    RUN_TEST(test_cleaning_set_threshold_valid);
    RUN_TEST(test_cleaning_set_threshold_minimum);
    RUN_TEST(test_cleaning_set_threshold_maximum);
    RUN_TEST(test_cleaning_set_threshold_below_minimum);
    RUN_TEST(test_cleaning_set_threshold_above_maximum);
    RUN_TEST(test_cleaning_set_threshold_zero);
    
    // Reminder Tests
    RUN_TEST(test_cleaning_reminder_not_due_initially);
    RUN_TEST(test_cleaning_reminder_not_due_below_threshold);
    RUN_TEST(test_cleaning_reminder_due_at_threshold);
    RUN_TEST(test_cleaning_reminder_due_above_threshold);
    RUN_TEST(test_cleaning_reminder_clears_after_reset);
    RUN_TEST(test_cleaning_reminder_with_different_thresholds);
    
    // Flash Wear Reduction Tests
    RUN_TEST(test_cleaning_unsaved_changes_after_brew);
    RUN_TEST(test_cleaning_unsaved_changes_after_threshold_change);
    RUN_TEST(test_cleaning_unsaved_changes_after_reset);
    RUN_TEST(test_cleaning_force_save_clears_flag);
    RUN_TEST(test_cleaning_short_brew_no_unsaved_changes);
    
    // Edge Cases
    RUN_TEST(test_cleaning_uint16_overflow_protection);
    RUN_TEST(test_cleaning_long_brew_duration);
    RUN_TEST(test_cleaning_max_uint32_duration);
    
    return UnityEnd();
}

