/**
 * Logging System Implementation
 * 
 * Provides structured logging with multiple levels and ESP32 forwarding.
 */

#include "logging.h"
#include "log_forward.h"
#include <stdio.h>
#include <string.h>

// =============================================================================
// Private State
// =============================================================================

static log_level_t g_log_level = LOG_LEVEL_INFO;  // Default to INFO
static bool g_forward_enabled = false;
static bool g_initialized = false;

// =============================================================================
// Configuration
// =============================================================================

void logging_init(void) {
    if (g_initialized) {
        return;
    }
    
    // Log forwarding is initialized separately in main.c
    // This is just for the logging level system
    g_log_level = LOG_LEVEL_INFO;
    g_forward_enabled = false;
    g_initialized = true;
}

void logging_set_level(log_level_t level) {
    if (level <= LOG_LEVEL_TRACE) {
        g_log_level = level;
    }
}

log_level_t logging_get_level(void) {
    return g_log_level;
}

void logging_set_forward_enabled(bool enable) {
    g_forward_enabled = enable;
    if (enable) {
        log_forward_set_enabled(true);
    } else {
        log_forward_set_enabled(false);
    }
}

bool logging_is_forward_enabled(void) {
    return g_forward_enabled;
}

// =============================================================================
// Logging Functions
// =============================================================================

void log_message_va(log_level_t level, const char* format, va_list args) {
    if (!g_initialized) {
        logging_init();
    }
    
    // Filter by level
    if (level > g_log_level) {
        return;
    }
    
    // Print to USB serial
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s", buffer);
    
    // Forward to ESP32 if enabled
    if (g_forward_enabled && log_forward_is_enabled()) {
        // Map log level to log_forward level
        uint8_t fwd_level = LOG_FWD_INFO;
        switch (level) {
            case LOG_LEVEL_ERROR: fwd_level = LOG_FWD_ERROR; break;
            case LOG_LEVEL_WARN:  fwd_level = LOG_FWD_WARN;  break;
            case LOG_LEVEL_INFO:  fwd_level = LOG_FWD_INFO;  break;
            case LOG_LEVEL_DEBUG: fwd_level = LOG_FWD_DEBUG; break;
            case LOG_LEVEL_TRACE: fwd_level = LOG_FWD_DEBUG; break;
        }
        log_forward_send(fwd_level, buffer);
    }
}

void log_message(log_level_t level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message_va(level, format, args);
    va_end(args);
}

// =============================================================================
// Utility Functions
// =============================================================================

const char* log_level_name(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_TRACE: return "TRACE";
        default:              return "UNKNOWN";
    }
}
