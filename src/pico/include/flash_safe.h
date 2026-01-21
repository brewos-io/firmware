/**
 * Pico Firmware - Flash Safety Utilities
 * 
 * Provides safe flash operations for both RP2040 and RP2350 (Pico 2).
 * 
 * On RP2350 / Pico SDK 2.0+, this wraps the SDK's built-in flash_safe_execute().
 * On older SDKs, it provides a compatible implementation.
 * 
 * CRITICAL: Flash operations require special handling because:
 * 1. The Pico executes code directly from flash (XIP - Execute In Place)
 * 2. During flash erase/program, flash cannot deliver instructions
 * 3. Any code executing from flash will hard fault during flash operations
 */

#ifndef FLASH_SAFE_H
#define FLASH_SAFE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// =============================================================================
// Flash Operation Parameters
// =============================================================================

typedef struct {
    uint32_t offset;        // Offset from start of flash (not XIP address)
    const uint8_t* data;    // Data to write (for program operations)
    size_t size;            // Size of operation (must be sector-aligned for erase)
} flash_op_params_t;

// =============================================================================
// Safe Flash Functions
// =============================================================================

/**
 * Initialize flash safety system.
 * Must be called from Core 0 before launching Core 1.
 * Sets up multicore lockout victim on current core.
 */
void flash_safe_init(void);

/**
 * Safely erase flash sectors.
 * 
 * Uses SDK's flash_safe_execute() internally which handles:
 * 1. Pausing the other core (multicore lockout)
 * 2. Disabling interrupts
 * 3. Running flash operations from RAM
 * 
 * @param offset Flash offset (must be sector-aligned, typically 4KB)
 * @param size   Number of bytes to erase (must be multiple of sector size)
 * @return true on success
 */
bool flash_safe_erase(uint32_t offset, size_t size);

/**
 * Safely program flash pages.
 * 
 * Uses SDK's flash_safe_execute() internally which handles safety.
 * 
 * @param offset Flash offset (must be page-aligned, typically 256 bytes)
 * @param data   Data to write (must be in RAM, not flash!)
 * @param size   Number of bytes to write (must be multiple of page size)
 * @return true on success
 */
bool flash_safe_program(uint32_t offset, const uint8_t* data, size_t size);

// =============================================================================
// Bootloader-specific Flash Functions (NO multicore lockout)
// =============================================================================
// These functions are for use ONLY during bootloader chunk reception.
// They write to the staging area which is separate from main firmware,
// so Core 1 can continue running from main firmware safely.
// They only disable interrupts (not multicore lockout) which is faster
// and avoids the multicore lockout timeout issues on RP2350.

/**
 * Erase flash for bootloader use (no multicore lockout).
 * Only disables interrupts, does not pause other core.
 * Safe for writing to staging area while Core 1 runs from main firmware.
 */
bool flash_bootloader_erase(uint32_t offset, size_t size);

/**
 * Program flash for bootloader use (no multicore lockout).
 * Only disables interrupts, does not pause other core.
 * Safe for writing to staging area while Core 1 runs from main firmware.
 */
bool flash_bootloader_program(uint32_t offset, const uint8_t* data, size_t size);

#endif // FLASH_SAFE_H

