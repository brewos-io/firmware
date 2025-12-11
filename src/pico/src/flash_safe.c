/**
 * ECM Pico Firmware - Flash Safety Implementation
 * 
 * Uses the Pico SDK's flash_safe_execute() for safe flash operations.
 * This is compatible with both RP2040 (SDK 1.5.1+) and RP2350 (SDK 2.0+).
 * 
 * The SDK's flash_safe_execute() handles:
 * - Multicore lockout (pausing the other core)
 * - Interrupt safety
 * - Running flash operations from RAM
 * 
 * RP2350 (Pico 2) Notes:
 * - Uses ARM Cortex-M33 cores with better atomic operations
 * - Stricter memory alignment requirements  
 * - SDK 2.0+ has improved flash safety helpers
 */

#include "flash_safe.h"
#include "config.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/flash.h"       // For flash_safe_execute()
#include "hardware/flash.h"

// =============================================================================
// State
// =============================================================================

static bool g_flash_safe_initialized = false;

// Default timeout for flash_safe_execute (10 seconds should be plenty)
#define FLASH_SAFE_TIMEOUT_MS 10000

// =============================================================================
// Internal Flash Operations (callbacks for flash_safe_execute)
// =============================================================================

/**
 * Internal erase callback - called by flash_safe_execute with safety guarantees.
 */
static void do_flash_erase(void* param) {
    flash_op_params_t* op = (flash_op_params_t*)param;
    flash_range_erase(op->offset, op->size);
}

/**
 * Internal program callback - called by flash_safe_execute with safety guarantees.
 */
static void do_flash_program(void* param) {
    flash_op_params_t* op = (flash_op_params_t*)param;
    flash_range_program(op->offset, op->data, op->size);
}

// =============================================================================
// Public API
// =============================================================================

void flash_safe_init(void) {
    if (g_flash_safe_initialized) return;
    
    // Initialize this core as a lockout "victim"
    // This allows the other core to pause us during flash operations.
    // Must be called from Core 0 before launching Core 1.
    multicore_lockout_victim_init();
    
    g_flash_safe_initialized = true;
    LOG_PRINT("Flash: Safety system initialized (using SDK flash_safe_execute)\n");
}

bool flash_safe_erase(uint32_t offset, size_t size) {
    // Validate offset is sector-aligned
    if (offset % FLASH_SECTOR_SIZE != 0) {
        LOG_PRINT("Flash: ERROR - Erase offset 0x%lx not sector-aligned\n", (unsigned long)offset);
        return false;
    }
    
    // Validate size is multiple of sector size
    if (size % FLASH_SECTOR_SIZE != 0) {
        DEBUG_PRINT("Flash safety: Erase size %zu not sector-aligned\n", size);
        return false;
    }
    
    // Validate bounds (check against PICO_FLASH_SIZE_BYTES)
    if (offset + size > PICO_FLASH_SIZE_BYTES) {
        DEBUG_PRINT("Flash safety: Erase exceeds flash bounds\n");
        return false;
    }
    
    flash_op_params_t op = {
        .offset = offset,
        .data = NULL,
        .size = size
    };
    
    // Use SDK's flash_safe_execute which handles multicore lockout and interrupt safety
    int result = flash_safe_execute(do_flash_erase, &op, FLASH_SAFE_TIMEOUT_MS);
    if (result != PICO_OK) {
        LOG_PRINT("Flash: ERROR - Erase failed (error %d)\n", result);
        return false;
    }
    
    return true;
}

bool flash_safe_program(uint32_t offset, const uint8_t* data, size_t size) {
    // Validate offset is page-aligned
    if (offset % FLASH_PAGE_SIZE != 0) {
        DEBUG_PRINT("Flash safety: Program offset 0x%lx not page-aligned\n", (unsigned long)offset);
        return false;
    }
    
    // Validate size is multiple of page size
    if (size % FLASH_PAGE_SIZE != 0) {
        DEBUG_PRINT("Flash safety: Program size %zu not page-aligned\n", size);
        return false;
    }
    
    // Validate data pointer
    if (!data) {
        DEBUG_PRINT("Flash safety: Program data is NULL\n");
        return false;
    }
    
    // Validate bounds
    if (offset + size > PICO_FLASH_SIZE_BYTES) {
        DEBUG_PRINT("Flash safety: Program exceeds flash bounds\n");
        return false;
    }
    
    flash_op_params_t op = {
        .offset = offset,
        .data = data,
        .size = size
    };
    
    // Use SDK's flash_safe_execute which handles multicore lockout and interrupt safety
    int result = flash_safe_execute(do_flash_program, &op, FLASH_SAFE_TIMEOUT_MS);
    if (result != PICO_OK) {
        DEBUG_PRINT("Flash safety: Program failed (error %d)\n", result);
        return false;
    }
    
    return true;
}
