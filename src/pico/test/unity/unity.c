/**
 * Unity Test Framework - Implementation
 */

#include "unity.h"

// Global test state
unity_state_t Unity;

void UnityBegin(const char* filename) {
    Unity.test_file = filename;
    Unity.tests_run = 0;
    Unity.tests_passed = 0;
    Unity.tests_failed = 0;
    printf("\n=== %s ===\n\n", filename);
}

int UnityEnd(void) {
    printf("\n-----------------------\n");
    printf("%d Tests, %d Passed, %d Failed\n", 
           Unity.tests_run, Unity.tests_passed, Unity.tests_failed);
    printf("-----------------------\n\n");
    return Unity.tests_failed;
}

void UnitySetTestFile(const char* file) {
    Unity.test_file = file;
}

void UnityDefaultTestRun(void (*func)(void), const char* name, int line) {
    Unity.test_name = name;
    Unity.test_line = line;
    Unity.current_test_failed = 0;
    Unity.tests_run++;
    
    // Run the test
    func();
    
    // Report result
    if (Unity.current_test_failed) {
        Unity.tests_failed++;
        printf("FAIL: %s\n", name);
    } else {
        Unity.tests_passed++;
        printf("PASS: %s\n", name);
    }
}

