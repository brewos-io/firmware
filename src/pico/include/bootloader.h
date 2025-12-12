#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Serial Bootloader for OTA Firmware Updates
 * 
 * Receives firmware over UART and writes to flash.
 * Protocol: [MAGIC: 0x55AA] [CHUNK_NUM: 4 bytes LE] [SIZE: 2 bytes LE] [DATA] [CHECKSUM: 1 byte]
 */

/**
 * Check if bootloader mode is active.
 * When active, other cores should pause their normal operations.
 * 
 * @return true if bootloader is receiving firmware
 */
bool bootloader_is_active(void);

/**
 * Prepare system for bootloader mode.
 * Call this BEFORE bootloader_receive_firmware() to signal other cores to pause.
 */
void bootloader_prepare(void);

/**
 * Exit bootloader mode (on failure).
 * Call this if bootloader_receive_firmware() returns an error to resume normal operation.
 */
void bootloader_exit(void);

// Bootloader result codes
typedef enum {
    BOOTLOADER_SUCCESS = 0,
    BOOTLOADER_ERROR_TIMEOUT = 1,
    BOOTLOADER_ERROR_INVALID_MAGIC = 2,
    BOOTLOADER_ERROR_INVALID_SIZE = 3,
    BOOTLOADER_ERROR_INVALID_CHUNK = 4,
    BOOTLOADER_ERROR_CHECKSUM = 5,
    BOOTLOADER_ERROR_FLASH_WRITE = 6,
    BOOTLOADER_ERROR_FLASH_ERASE = 7,
    BOOTLOADER_ERROR_UNKNOWN = 8
} bootloader_result_t;

/**
 * Enter bootloader mode and receive firmware
 * 
 * This function:
 * 1. Sends acknowledgment to ESP32
 * 2. Switches to bootloader protocol mode
 * 3. Receives firmware chunks over UART
 * 4. Writes firmware to flash
 * 5. Verifies checksums
 * 6. Resets on success or returns error
 * 
 * @return bootloader_result_t - Result of the bootloader operation
 * 
 * Note: This function does not return on success (resets the device).
 * On error, it returns to allow error reporting.
 */
bootloader_result_t bootloader_receive_firmware(void);

/**
 * Get bootloader status message
 * 
 * @param result - Bootloader result code
 * @return const char* - Human-readable status message
 */
const char* bootloader_get_status_message(bootloader_result_t result);

#endif // BOOTLOADER_H

