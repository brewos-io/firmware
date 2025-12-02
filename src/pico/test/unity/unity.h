/**
 * Unity Test Framework - Minimal Version
 * 
 * Lightweight test framework for C, optimized for embedded systems.
 * Based on Unity by ThrowTheSwitch.org
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// =============================================================================
// Configuration
// =============================================================================

#ifndef UNITY_FLOAT_PRECISION
#define UNITY_FLOAT_PRECISION 0.00001f
#endif

// =============================================================================
// Test State
// =============================================================================

typedef struct {
    const char* test_name;
    const char* test_file;
    int test_line;
    int tests_run;
    int tests_passed;
    int tests_failed;
    int current_test_failed;
} unity_state_t;

extern unity_state_t Unity;

// =============================================================================
// Core Functions
// =============================================================================

void UnityBegin(const char* filename);
int UnityEnd(void);
void UnitySetTestFile(const char* file);
void UnityDefaultTestRun(void (*func)(void), const char* name, int line);

// =============================================================================
// Test Macros
// =============================================================================

#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_NULL(pointer) \
    do { \
        if ((pointer) != NULL) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected NULL\n", __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(pointer) \
    do { \
        if ((pointer) == NULL) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected not NULL\n", __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) TEST_ASSERT_EQUAL(expected, actual)

#define TEST_ASSERT_EQUAL_UINT8(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected 0x%02X, got 0x%02X\n", __FILE__, __LINE__, (unsigned)(expected), (unsigned)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_UINT16(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected 0x%04X, got 0x%04X\n", __FILE__, __LINE__, (unsigned)(expected), (unsigned)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_UINT32(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected 0x%08lX, got 0x%08lX\n", __FILE__, __LINE__, (unsigned long)(expected), (unsigned long)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_INT16(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) \
    do { \
        float _diff = fabsf((expected) - (actual)); \
        if (_diff > UNITY_FLOAT_PRECISION) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected %.6f, got %.6f (diff=%.6f)\n", __FILE__, __LINE__, (double)(expected), (double)(actual), (double)_diff); \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    do { \
        float _diff = fabsf((expected) - (actual)); \
        if (_diff > (delta)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected %.6f +/- %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(delta), (double)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    do { \
        if (!((actual) > (threshold))) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected > %d, got %d\n", __FILE__, __LINE__, (int)(threshold), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    do { \
        if (!((actual) < (threshold))) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected < %d, got %d\n", __FILE__, __LINE__, (int)(threshold), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual) \
    do { \
        if (!((actual) >= (threshold))) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected >= %d, got %d\n", __FILE__, __LINE__, (int)(threshold), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_LESS_OR_EQUAL(threshold, actual) \
    do { \
        if (!((actual) <= (threshold))) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected <= %d, got %d\n", __FILE__, __LINE__, (int)(threshold), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, (expected), (actual)); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    do { \
        if (memcmp((expected), (actual), (len)) != 0) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Memory mismatch in %zu bytes\n", __FILE__, __LINE__, (size_t)(len)); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE_MESSAGE(condition, message) \
    do { \
        if (!(condition)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: %s\n", __FILE__, __LINE__, (message)); \
        } \
    } while(0)

#define TEST_FAIL_MESSAGE(message) \
    do { \
        Unity.current_test_failed = 1; \
        printf("  FAIL: %s:%d: %s\n", __FILE__, __LINE__, (message)); \
    } while(0)

#define TEST_FAIL() TEST_FAIL_MESSAGE("Test failed")

#define TEST_PASS() do {} while(0)

#define TEST_IGNORE() \
    do { \
        printf("  IGNORE: %s\n", Unity.test_name); \
        return; \
    } while(0)

#define TEST_IGNORE_MESSAGE(message) \
    do { \
        printf("  IGNORE: %s - %s\n", Unity.test_name, (message)); \
        return; \
    } while(0)

// NaN check for floats
#define TEST_ASSERT_FLOAT_IS_NAN(value) \
    do { \
        if (!isnan(value)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected NaN, got %.6f\n", __FILE__, __LINE__, (double)(value)); \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_IS_NOT_NAN(value) \
    do { \
        if (isnan(value)) { \
            Unity.current_test_failed = 1; \
            printf("  FAIL: %s:%d: Expected not NaN\n", __FILE__, __LINE__); \
        } \
    } while(0)

#endif // UNITY_H

