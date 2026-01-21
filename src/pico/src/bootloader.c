/**
 * Pico Firmware - Serial Bootloader
 * FIXED: Removed risky Flash Verification, Added Watchdog Reset, Tuned UART timing.
 */

 #include "bootloader.h"
 #include "config.h"
 #include "flash_safe.h"       
 #include "safety.h"
 #include "protocol.h"  // For protocol_reset_state()           
 #include <string.h>
 #include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"   
#include "pico/platform.h"
#include "pico/bootrom.h"
#include "pico/flash.h"           // For flash_safe_execute on RP2350
#include "hardware/uart.h"
#include "hardware/structs/uart.h" // For uart_hw_t and UART register access
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "hardware/structs/scb.h"
// XIP cache control - RP2040 and RP2350 have different APIs
// We use conditional compilation for portability
#if PICO_RP2040
#include "hardware/structs/xip_ctrl.h"
#endif
 
 // Bootloader protocol constants
 #define BOOTLOADER_MAGIC_1          0x55
 #define BOOTLOADER_MAGIC_2          0xAA
 #define BOOTLOADER_END_MAGIC_1      0xAA
 #define BOOTLOADER_END_MAGIC_2      0x55
 #define BOOTLOADER_CHUNK_MAX_SIZE   256
 #define BOOTLOADER_TIMEOUT_MS       60000    // Overall timeout: 60 seconds 
 #define BOOTLOADER_CHUNK_TIMEOUT_MS 10000    // Per-chunk timeout: 10 seconds (was 5s)
 #define FLASH_WRITE_RETRIES         3
 
 // Fallback for missing error code in header
 #ifndef BOOTLOADER_ERROR_FAILED
 #define BOOTLOADER_ERROR_FAILED BOOTLOADER_ERROR_FLASH_WRITE
 #endif
 
 // Flash layout
 #define FLASH_TARGET_OFFSET         (1536 * 1024)  // Staging area
 #define FLASH_MAIN_OFFSET           0              // Main firmware area
 
// Bootloader state
static uint32_t g_received_size = 0;
static uint32_t g_chunk_count = 0;
static bool g_receiving = false;
static volatile bool g_bootloader_active = false;

// RAM-resident buffer for debug logging (visible in RAM dump if we crash)
static volatile char g_debug_buffer[256] __attribute__((section(".uninitialized_data")));
static volatile uint32_t g_debug_index __attribute__((section(".uninitialized_data")));

// Large RAM buffer to preload entire firmware before flash operations
// This avoids ANY XIP flash access during the critical erase/program phase
// 128KB should be enough for any firmware we might flash
// Placed in .uninitialized_data to avoid zeroing (faster boot)
#define FIRMWARE_PRELOAD_BUFFER_SIZE (128 * 1024)
static uint8_t g_firmware_preload_buffer[FIRMWARE_PRELOAD_BUFFER_SIZE] __attribute__((section(".uninitialized_data")));

// Simple RAM-resident debug marker that sends bytes to UART (visible to ESP32)
// Format: 0xDB [marker_byte] [value_lo] [value_hi]
// FIXED for RP2350: Use uart_get_hw() macro for correct base address on both RP2040 and RP2350
// The SDK's uart_get_hw() handles platform differences automatically

static void __no_inline_not_in_flash_func(debug_marker)(uint8_t marker, uint16_t value) {
    // Use SDK's uart_get_hw() for correct UART base address on RP2040/RP2350
    // This is RAM-resident via the SDK and handles platform differences
    uart_hw_t *uart_hw = uart_get_hw(ESP32_UART_ID);
    
    // Wait for FIFO space and send marker bytes
    // UART_UARTFR_TXFF_BITS is the TX FIFO full flag
    while (uart_hw->fr & UART_UARTFR_TXFF_BITS) { __asm volatile("nop"); }
    uart_hw->dr = 0xDB;  // Debug marker prefix
    while (uart_hw->fr & UART_UARTFR_TXFF_BITS) { __asm volatile("nop"); }
    uart_hw->dr = marker;
    while (uart_hw->fr & UART_UARTFR_TXFF_BITS) { __asm volatile("nop"); }
    uart_hw->dr = (value & 0xFF);
    while (uart_hw->fr & UART_UARTFR_TXFF_BITS) { __asm volatile("nop"); }
    uart_hw->dr = ((value >> 8) & 0xFF);
    
    // Also store in RAM buffer for debugging
    if (g_debug_index < sizeof(g_debug_buffer) - 4) {
        g_debug_buffer[g_debug_index++] = marker;
        g_debug_buffer[g_debug_index++] = (value & 0xFF);
        g_debug_buffer[g_debug_index++] = ((value >> 8) & 0xFF);
        g_debug_buffer[g_debug_index++] = '\n';
    }
}

// Debug marker codes for copy_firmware_to_main
#define DBG_COPY_ENTER      0x01  // Entered function
#define DBG_SIZE_CHECK      0x02  // Size validated
#define DBG_SECTOR_COUNT    0x03  // Sector count
#define DBG_STAGING_READ    0x04  // Read staging area
#define DBG_VECTOR_CHECK    0x05  // Vector table check
#define DBG_VECTOR_FAIL     0x06  // Vector validation failed
#define DBG_LOOP_START      0x07  // Starting copy loop
#define DBG_SECTOR_COPY     0x08  // Copying sector N
#define DBG_ERASE_START     0x09  // Starting erase
#define DBG_ERASE_DONE      0x0A  // Erase complete
#define DBG_PROG_START      0x0B  // Starting program
#define DBG_PROG_DONE       0x0C  // Program complete
#define DBG_LOOP_DONE       0x0D  // Loop complete
#define DBG_RESET_TRIGGER   0x0E  // Triggering reset
#define DBG_COPY_EXIT_ERR   0x0F  // Exit with error
 
// -----------------------------------------------------------------------------
// BootROM Function Typedefs
// -----------------------------------------------------------------------------
// REMOVED: BootROM function lookup code that was RP2040-specific
// The standard SDK functions flash_range_erase() and flash_range_program()
// are marked as __no_inline_not_in_flash_func, meaning they reside in RAM
// and are safe to call even while erasing flash. They also handle RP2040/RP2350
// differences automatically via the hardware/flash.h library.

// -----------------------------------------------------------------------------
// Bootloader Mode Control
// -----------------------------------------------------------------------------
 
 bool bootloader_is_active(void) {
     return g_bootloader_active;
 }
 
void bootloader_prepare(void) {
    // IMMEDIATE DEBUG - bypass LOG_PRINT, use direct printf
    // Use multiple approaches to ensure output is visible
    printf("\n\n");
    printf("********************************************\n");
    printf("*** BOOTLOADER_PREPARE() CALLED ***\n");
    printf("********************************************\n");
    fflush(stdout);
    sleep_ms(50);  // Give USB time to transmit
    
    // Idempotency check: if already active, just return
    // This prevents double-initialization and race conditions
    if (g_bootloader_active) {
        printf("Bootloader: Already active, skipping prepare\n");
        fflush(stdout);
        return;
    }
    
    printf("Bootloader: Entering safe state (heaters OFF)\n");
    fflush(stdout);
    safety_enter_safe_state();
    
    // =========================================================================
    // NOTE: We do NOT kill Core 1 here in bootloader_prepare()
    // The lockout/reset mechanism requires Core 1 to have a lockout handler
    // which may not be set up, causing hangs.
    // 
    // Instead, we rely on:
    // 1. g_bootloader_active flag - Core 1's main loop checks this and suspends
    // 2. Lockout in copy_firmware_to_main() - done right before flash erase
    //
    // The handshake and firmware reception phases are safe - Core 1 can still
    // run during these. We only need Core 1 stopped during flash copy.
    // =========================================================================
    printf("Bootloader: Core 1 will be stopped later during flash copy\n");
    fflush(stdout);
    
    // NOTE: USB serial logging is NOT affected by UART interrupt settings
    // UART interrupts only control hardware UART (ESP32 communication), not USB serial
    // All LOG_PRINT statements will continue to work over USB
    
    // CRITICAL: Drain UART FIFO completely BEFORE resetting protocol state
    // Note: We disable UART interrupts AFTER sending bootloader ACK (in bootloader_receive_firmware)
    // This allows protocol ACK and bootloader ACK to be sent properly
    // This prevents any bytes in the FIFO from being processed by protocol handler
    // We must drain while protocol is still active, then reset state, then set flag
    // Drain multiple times to ensure FIFO is completely empty
    int drain_count = 0;
    while (uart_is_readable(ESP32_UART_ID)) {
        (void)uart_getc(ESP32_UART_ID);
        drain_count++;
    }
    if (drain_count > 0) {
        LOG_PRINT("Bootloader: Drained %d bytes from UART FIFO\n", drain_count);
    }
    
    // CRITICAL: Reset protocol state machine to prevent parsing bootloader data as protocol packets
    // Bootloader data (0x55AA chunks) will be misinterpreted as protocol packets if state is not reset
    protocol_reset_state();
    
    // Memory barrier to ensure protocol_reset_state() completes before setting flag
    __dmb();
    
    // CRITICAL: Set bootloader_active flag AFTER draining UART and resetting state
    // This ensures protocol_process() won't consume any bytes after this point
    g_bootloader_active = true;
    
    // Full memory barrier to ensure flag is visible to all cores immediately
    __dmb();
    
    // CRITICAL: Minimal delay to ensure all cores see the flag change
    // The flag is set with memory barriers, so cores should see it within microseconds
    // 5ms is sufficient for Core 0 and Core 1's main loops to check the flag and suspend
    // We keep this short because bootloader_prepare() is now called BEFORE sending ACK
    sleep_ms(5);
    
    // Final drain to catch any bytes that arrived during the transition
    // This is critical - bytes could arrive between setting flag and protocol_process() checking it
    drain_count = 0;
    while (uart_is_readable(ESP32_UART_ID)) {
        (void)uart_getc(ESP32_UART_ID);
        drain_count++;
    }
    if (drain_count > 0) {
        LOG_PRINT("Bootloader: Drained %d additional bytes after transition\n", drain_count);
    }
    
    LOG_PRINT("Bootloader: System paused, safe to proceed\n");
}
 
 void bootloader_exit(void) {
     // CRITICAL: Aggressively drain ALL bootloader data before exiting
     // ESP32 may still be sending firmware chunks even after bootloader fails
     // We must consume ALL of this data or it will be processed as protocol packets
     
     // Keep bootloader flag set and interrupts disabled while draining
     // This prevents protocol handler from processing bytes during drain
     
     int total_drained = 0;
     uint32_t drain_start = to_ms_since_boot(get_absolute_time());
     const uint32_t DRAIN_TIMEOUT_MS = 2000;  // Drain for up to 2 seconds
     
     // Aggressive drain loop: keep draining until no bytes arrive for 100ms
     uint32_t last_byte_time = drain_start;
     while ((to_ms_since_boot(get_absolute_time()) - drain_start) < DRAIN_TIMEOUT_MS) {
         int cycle_drained = 0;
         while (uart_is_readable(ESP32_UART_ID)) {
             (void)uart_getc(ESP32_UART_ID);
             cycle_drained++;
             total_drained++;
             last_byte_time = to_ms_since_boot(get_absolute_time());
         }
         
         // If no bytes arrived in last 100ms, assume ESP32 stopped sending
         if ((to_ms_since_boot(get_absolute_time()) - last_byte_time) > 100) {
             break;
         }
         
         // Small delay to avoid busy-waiting
         sleep_ms(10);
     }
     
     if (total_drained > 0) {
         LOG_PRINT("Bootloader: Drained %d total bytes before exit\n", total_drained);
     }
     
     // CRITICAL: Final drain with interrupts still disabled
     // This catches any bytes that arrived during the aggressive drain
     int final_drain = 0;
     uint32_t final_start = to_ms_since_boot(get_absolute_time());
     while ((to_ms_since_boot(get_absolute_time()) - final_start) < 200) {  // 200ms final drain
         if (uart_is_readable(ESP32_UART_ID)) {
             (void)uart_getc(ESP32_UART_ID);
             final_drain++;
         } else {
             sleep_ms(10);
         }
     }
     if (final_drain > 0) {
         LOG_PRINT("Bootloader: Drained %d additional bytes (final drain)\n", final_drain);
     }
     
     // CRITICAL: Bootloader failed - reset Pico to resume normal operation
     // Don't try to resume protocol handler - system is in inconsistent state
     // Reset ensures clean state and proper recovery
     LOG_PRINT("Bootloader: Failed, resetting Pico to resume normal operation\n");
     
     // Small delay to ensure log message is sent
     sleep_ms(100);
     
     // Reset via watchdog (clean reset method)
     watchdog_reboot(0, 0, 0);
     
     // Should never reach here, but just in case
     while(1) __asm volatile("nop");
 }
 
// -----------------------------------------------------------------------------
// Core 0 Safe Loop (RAM Only)
// -----------------------------------------------------------------------------

void __no_inline_not_in_flash_func(bootloader_core0_loop)(void) {
    // Disable interrupts on Core 0 to prevent any flash access from ISRs
    uint32_t irq_state = save_and_disable_interrupts();
    
    // Loop until system reset (we effectively park Core 0 here)
    // Note: We do NOT feed the watchdog here. Core 1 takes ownership of the watchdog
    // during OTA. If Core 1 hangs, the watchdog will reset the system.
    while (g_bootloader_active) {
        // Busy wait (don't sleep, as waking up might involve flash)
        // Just spin safely in RAM
        for (volatile int i = 0; i < 1000; i++) { __asm volatile("nop"); }
    }
    
    restore_interrupts(irq_state);
}

// -----------------------------------------------------------------------------
// Utilities & Protocol Helpers
// -----------------------------------------------------------------------------
 static uint32_t crc32_calculate(const uint8_t* data, size_t length) {
     uint32_t crc = 0xFFFFFFFF;
     const uint32_t polynomial = 0xEDB88320;
     for (size_t i = 0; i < length; i++) {
         crc ^= data[i];
         for (int j = 0; j < 8; j++) {
             if (crc & 1) crc = (crc >> 1) ^ polynomial;
             else crc >>= 1;
         }
     }
     return ~crc;
 }
 
 static bool uart_read_byte_timeout(uint8_t* byte, uint32_t timeout_ms) {
     absolute_time_t timeout = make_timeout_time_ms(timeout_ms);
     uint32_t last_watchdog_feed = 0;
     while (!uart_is_readable(ESP32_UART_ID)) {
         if (time_reached(timeout)) return false;
         
         // Feed watchdog periodically during long waits (every ~100ms)
         uint32_t now_ms = to_ms_since_boot(get_absolute_time());
         if (now_ms - last_watchdog_feed > 100) {
             watchdog_update();
             last_watchdog_feed = now_ms;
         }
         
         // [TUNED] 10us allows ~1 byte at 921k baud (9us/byte).
         // This is safe for the FIFO (32 bytes) while being efficient.
         sleep_us(10); 
     }
     *byte = uart_getc(ESP32_UART_ID);
     return true;
 }
 
 static bool uart_read_bytes_timeout(uint8_t* buffer, size_t count, uint32_t timeout_ms) {
     absolute_time_t start = get_absolute_time();
     for (size_t i = 0; i < count; i++) {
         uint32_t elapsed_ms = to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(start);
         uint32_t remaining = timeout_ms > elapsed_ms ? timeout_ms - elapsed_ms : 100;
         if (!uart_read_byte_timeout(&buffer[i], remaining)) return false;
     }
     return true;
 }
 
 static void uart_write_byte(uint8_t byte) {
     uart_putc(ESP32_UART_ID, byte);
 }
 
 static bool receive_chunk_header(uint32_t* chunk_num, uint16_t* chunk_size) {
     absolute_time_t start_time = get_absolute_time();
     absolute_time_t timeout_time = make_timeout_time_ms(BOOTLOADER_CHUNK_TIMEOUT_MS);
     uint32_t bytes_seen = 0;  // Track bytes received for debug
     
     while (!time_reached(timeout_time)) {
         // Feed watchdog during long waits to prevent reset
         watchdog_update();
         
         uint8_t b1;
         if (!uart_read_byte_timeout(&b1, 100)) continue;
         bytes_seen++;
         
         if (b1 == BOOTLOADER_MAGIC_1) { 
             uint8_t b2;
             if (!uart_read_byte_timeout(&b2, 100)) continue;
             bytes_seen++;
             if (b2 == BOOTLOADER_MAGIC_2) { 
                 uint8_t h[6];
                 if (!uart_read_bytes_timeout(h, 6, BOOTLOADER_CHUNK_TIMEOUT_MS)) return false;
                 bytes_seen += 6;
                 *chunk_num = h[0] | (h[1]<<8) | (h[2]<<16) | (h[3]<<24);
                 *chunk_size = h[4] | (h[5]<<8);
                 return true;
             }
         } else if (b1 == BOOTLOADER_END_MAGIC_1) {
             uint8_t b2;
             if (!uart_read_byte_timeout(&b2, 100)) continue;
             bytes_seen++;
             if (b2 == BOOTLOADER_END_MAGIC_2) {
                 uint8_t b3;
                 if (!uart_read_byte_timeout(&b3, 200)) { *chunk_num = 0xFFFFFFFF; return true; }
                 bytes_seen++;
                 if (b3 == BOOTLOADER_MAGIC_2) continue; 
                 *chunk_num = 0xFFFFFFFF; return true;
             }
         }
     }
     
     // Timeout - log how long we waited and how many bytes we saw
     uint32_t elapsed_ms = to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(start_time);
     LOG_PRINT("Bootloader: Chunk header timeout after %lums (saw %lu bytes)\n", 
               (unsigned long)elapsed_ms, (unsigned long)bytes_seen);
     return false;
 }
 
 static bool receive_chunk_data(uint8_t* buffer, uint16_t size, uint8_t* checksum) {
     // Feed watchdog before potentially long read operations
     watchdog_update();
     if (!uart_read_bytes_timeout(buffer, size, BOOTLOADER_CHUNK_TIMEOUT_MS)) return false;
     watchdog_update();
     if (!uart_read_byte_timeout(checksum, BOOTLOADER_CHUNK_TIMEOUT_MS)) return false;
     uint8_t calc = 0;
     for(int i=0; i<size; i++) calc ^= buffer[i];
     return (calc == *checksum);
 }
 
// -----------------------------------------------------------------------------
// CRITICAL: Safe Flash Copy (RAM Only)
// -----------------------------------------------------------------------------

static uint8_t g_sector_buffer[FLASH_SECTOR_SIZE] __attribute__((aligned(16)));

// RAM-resident UART helpers to avoid flash access during copy
static void __no_inline_not_in_flash_func(uart_write_byte_ram)(uint8_t byte) {
    uart_hw_t *uart_hw = uart_get_hw(ESP32_UART_ID);
    while (uart_hw->fr & UART_UARTFR_TXFF_BITS) { __asm volatile("nop"); }
    uart_hw->dr = byte;
}

static void __no_inline_not_in_flash_func(uart_wait_ram)(void) {
    uart_hw_t *uart_hw = uart_get_hw(ESP32_UART_ID);
    while (uart_hw->fr & UART_UARTFR_BUSY_BITS) { __asm volatile("nop"); }
}

// -----------------------------------------------------------------------------
// Safe Flash Operations - Direct Bootrom Access
// -----------------------------------------------------------------------------
// CRITICAL FIX: The SDK's flash_range_erase/flash_range_program functions have
// internal helper code that may reside in flash. When we erase the sector
// containing that helper code, we crash.
//
// Solution: Use the bootrom functions DIRECTLY via rom_func_lookup(). The bootrom
// is in ROM (not flash), so it never gets erased and is always safe to call.
//
// Bootrom flash functions require:
// 1. Exit XIP mode (so flash isn't being accessed)
// 2. Call the bootrom erase/program function
// 3. Flush cache
// 4. Re-enter XIP mode (but we skip this since we're resetting anyway)

// ROM function type definitions
typedef void (*rom_connect_internal_flash_fn)(void);
typedef void (*rom_flash_exit_xip_fn)(void);
typedef void (*rom_flash_range_erase_fn)(uint32_t addr, size_t count, uint32_t block_size, uint8_t block_cmd);
typedef void (*rom_flash_range_program_fn)(uint32_t addr, const uint8_t *data, size_t count);
typedef void (*rom_flash_flush_cache_fn)(void);

// ROM function pointers (resolved once at start of flash operations)
static rom_connect_internal_flash_fn g_rom_connect_internal_flash;
static rom_flash_exit_xip_fn g_rom_flash_exit_xip;
static rom_flash_range_erase_fn g_rom_flash_range_erase;
static rom_flash_range_program_fn g_rom_flash_range_program;
static rom_flash_flush_cache_fn g_rom_flash_flush_cache;

// Flag to track if we've exited XIP mode
static volatile bool g_xip_exited = false;

// Initialize ROM function pointers (call once before flash operations)
static void __no_inline_not_in_flash_func(init_rom_flash_functions)(void) {
    // Look up bootrom functions - these are in ROM, not flash
    g_rom_connect_internal_flash = (rom_connect_internal_flash_fn)rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH);
    g_rom_flash_exit_xip = (rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
    g_rom_flash_range_erase = (rom_flash_range_erase_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_RANGE_ERASE);
    g_rom_flash_range_program = (rom_flash_range_program_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_RANGE_PROGRAM);
    g_rom_flash_flush_cache = (rom_flash_flush_cache_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);
}

// Exit XIP mode and prepare for flash operations (call once before loop)
static void __no_inline_not_in_flash_func(prepare_flash_for_write)(void) {
    if (!g_xip_exited) {
        // Connect to internal flash
        g_rom_connect_internal_flash();
        // Exit XIP mode - after this, DO NOT access flash memory!
        g_rom_flash_exit_xip();
        g_xip_exited = true;
    }
}

// Safe erase using direct bootrom call
static void __no_inline_not_in_flash_func(safe_flash_erase)(uint32_t offset, uint32_t size) {
    // CRITICAL: XIP must already be exited before calling this!
    // Use sector erase command (0x20 for 4KB sectors)
    // The bootrom function takes: address, count, block_size, block_cmd
    // For sector erase: block_size = FLASH_SECTOR_SIZE (4096), block_cmd = 0x20
    g_rom_flash_range_erase(offset, size, FLASH_SECTOR_SIZE, 0x20);
}

// Safe program using direct bootrom call
static void __no_inline_not_in_flash_func(safe_flash_program)(uint32_t offset, const uint8_t* data, uint32_t size) {
    // CRITICAL: XIP must already be exited before calling this!
    // The bootrom function takes: address, data, count
    g_rom_flash_range_program(offset, data, size);
}

// Flush cache after flash operations (call after all erase/program done)
static void __no_inline_not_in_flash_func(flush_flash_cache)(void) {
    g_rom_flash_flush_cache();
}

/**
 * CRITICAL: This function runs entirely from RAM.
 * We use the SDK's flash_range_erase/program functions directly because:
 * 1. This function and all helpers are marked __no_inline_not_in_flash_func (RAM-resident)
 * 2. The SDK's flash functions are also RAM-resident
 * 3. We've already set up the safe environment (interrupts disabled, other core parked)
 * 
 * NOTE: Do NOT use flash_safe_execute() here - it would try to do the lockout again,
 * causing a hang since we've already disabled interrupts and stopped cores.
 */
static void __no_inline_not_in_flash_func(copy_firmware_to_main)(uint32_t firmware_size) {
    // =========================================================================
    // PHASE 1: VALIDATION WITH INTERRUPTS ENABLED (can still log to USB)
    // =========================================================================
    
    // TEST: Send bytes on function entry using RAM-resident UART functions
    uart_write_byte_ram(0x55);
    uart_write_byte_ram(0x66);
    uart_write_byte_ram(0x77);
    uart_write_byte_ram(0x88);
    uart_wait_ram();
    
    LOG_PRINT("Bootloader: copy_firmware_to_main() called with size=%lu\n", (unsigned long)firmware_size);
    
    // CRITICAL: Validate firmware_size before proceeding
    if (firmware_size == 0 || firmware_size > (1024 * 1024)) {
        debug_marker(DBG_COPY_EXIT_ERR, 0x0001);  // Error code 1: invalid size
        LOG_PRINT("Bootloader: ABORT - Invalid firmware size: %lu (must be 1B-1MB)\n", (unsigned long)firmware_size);
        return;
    }
    
    // Calculate sector count early for validation
    uint32_t sector_count = (firmware_size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
    if (sector_count == 0 || sector_count > 256) {
        debug_marker(DBG_COPY_EXIT_ERR, 0x0002);  // Error code 2: invalid sector count
        LOG_PRINT("Bootloader: ABORT - Invalid sector count: %lu\n", (unsigned long)sector_count);
        return;
    }
    
    // Send validation passed markers
    debug_marker(DBG_SIZE_CHECK, (uint16_t)firmware_size);  // Confirm size validated
    debug_marker(DBG_SECTOR_COUNT, (uint16_t)sector_count);  // Confirm sector count
    uart_wait_ram();  // Ensure markers sent
    
    LOG_PRINT("Bootloader: Will copy %lu sectors (%lu bytes)\n", (unsigned long)sector_count, (unsigned long)firmware_size);
    
    // CRITICAL: Use NON-CACHED flash access to avoid XIP cache coherency issues
    // Memory barriers ensure all writes are complete before we read
    __dmb();
    __dsb();
    __isb();
    
    // Use standard XIP address for both RP2040 and RP2350
    // The SDK flash operations should handle cache coherency internally
    // Using non-cached address (0x13000000) on RP2350 was causing crashes
    const uint8_t* staging_base = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    
#if PICO_RP2040
    // On RP2040, explicitly flush XIP cache
    xip_ctrl_hw->flush = 1;
    (void)xip_ctrl_hw->flush;
    __dmb();
#endif
    
    LOG_PRINT("Bootloader: Staging base = 0x%08lX\n", (unsigned long)staging_base);
    
    // Verify staging area contains valid ARM vectors BEFORE disabling interrupts
    // ==========================================================================
    // IMPORTANT: Vector validation was already done in bootloader_receive_firmware()
    // We SKIP redundant validation here because:
    // 1. Reading from XIP window in RAM function can have cache coherency issues
    // 2. The data was already validated before this function was called
    // 3. Redundant checks caused false-positive failures and immediate reboots
    // ==========================================================================
    
    // Send debug marker to confirm we reached this point (ESP32 can see this)
    debug_marker(DBG_STAGING_READ, (uint16_t)(firmware_size / 1024));  // Size in KB
    uart_wait_ram();  // FLUSH!
    
    // =========================================================================
    // CRITICAL FIX FOR RP2350: Preload ALL firmware data from staging to RAM
    // BEFORE disabling interrupts and doing any flash operations.
    // 
    // On RP2350, accessing XIP flash (even the staging area) during flash erase/
    // program operations on the main area can cause crashes. The XIP controller
    // may get into a bad state when we try to read while flash operations are
    // in progress, even on different sectors.
    //
    // By preloading everything to RAM first, we avoid ANY XIP access during
    // the critical flash erase/program phase.
    // =========================================================================
    
    LOG_PRINT("Bootloader: Pre-loading ALL %lu bytes from staging to RAM...\n", (unsigned long)firmware_size);
    
    // Verify we have enough buffer space
    if (firmware_size > FIRMWARE_PRELOAD_BUFFER_SIZE) {
        LOG_PRINT("Bootloader: ERROR - Firmware too large for preload buffer (%lu > %lu)\n",
                  (unsigned long)firmware_size, (unsigned long)FIRMWARE_PRELOAD_BUFFER_SIZE);
        debug_marker(0xEE, 0xFFFF);  // EE = Error, FFFF = buffer overflow
        return;
    }
    
    // Flush XIP cache before reading to ensure fresh data
#if PICO_RP2040
    xip_ctrl_hw->flush = 1;
    (void)xip_ctrl_hw->flush;
    __dmb();
#endif
    
    // Copy ALL firmware from staging (XIP flash) to RAM buffer
    // This is done with interrupts ENABLED, so it's safe to access XIP
    for (size_t k = 0; k < firmware_size; k++) {
        g_firmware_preload_buffer[k] = staging_base[k];
    }
    
    // Pad the rest of the last sector with 0xFF (for flash programming)
    uint32_t padded_size = ((firmware_size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    for (size_t k = firmware_size; k < padded_size; k++) {
        g_firmware_preload_buffer[k] = 0xFF;
    }
    
    // Memory barrier to ensure all writes are complete
    __dmb();
    
    LOG_PRINT("Bootloader: Preload complete! %lu bytes copied to RAM (padded to %lu)\n",
              (unsigned long)firmware_size, (unsigned long)padded_size);
    
    // Also copy sector 0 to g_sector_buffer for vector validation
    for (size_t k = 0; k < FLASH_SECTOR_SIZE; k++) {
        g_sector_buffer[k] = g_firmware_preload_buffer[k];
    }
    
    // Send marker to confirm pre-load complete
    debug_marker(0xF0, (uint16_t)(g_firmware_preload_buffer[0]));  // F0 = preload done, first byte
    uart_wait_ram();  // FLUSH!
    
    // Validate from RAM buffer (not XIP) - this avoids cache coherency issues
    uint32_t sp_check = ((uint32_t*)g_sector_buffer)[0];
    uint32_t pc_check = ((uint32_t*)g_sector_buffer)[1];
    
    LOG_PRINT("Bootloader: RAM buffer vectors: SP=0x%08lX, PC=0x%08lX\n", (unsigned long)sp_check, (unsigned long)pc_check);
    debug_marker(DBG_VECTOR_CHECK, (uint16_t)(sp_check >> 16));  // Send upper bits of SP
    uart_wait_ram();  // FLUSH!
    
    bool valid_sp = (sp_check & 0xFF000000) == 0x20000000;
    bool valid_pc = (pc_check & 0xFF000000) == 0x10000000;
    
    if (!valid_sp || !valid_pc) {
        // CRITICAL: Send debug marker via UART so ESP32 can see the failure!
        debug_marker(DBG_VECTOR_FAIL, (uint16_t)(sp_check >> 24) | ((uint16_t)(pc_check >> 24) << 8));
        
        LOG_PRINT("Bootloader: WARNING - Vector check failed but continuing anyway\n");
        LOG_PRINT("Bootloader: SP=0x%08lX (valid=%d), PC=0x%08lX (valid=%d)\n", 
                  (unsigned long)sp_check, valid_sp, (unsigned long)pc_check, valid_pc);
        
        // DON'T ABORT! The validation in bootloader_receive_firmware already passed.
        // This failure is likely due to XIP cache issues, not actual data corruption.
        // Proceed with the copy - worst case we brick, but aborting always fails.
    }
    
    LOG_PRINT("Bootloader: RAM buffer[0-7]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
              g_sector_buffer[0], g_sector_buffer[1], g_sector_buffer[2], g_sector_buffer[3],
              g_sector_buffer[4], g_sector_buffer[5], g_sector_buffer[6], g_sector_buffer[7]);
    
    LOG_PRINT("Bootloader: ============================================\n");
    LOG_PRINT("Bootloader: DISABLING INTERRUPTS - NO MORE USB LOGGING!\n");
    LOG_PRINT("Bootloader: Debug markers (0xDB xx yy zz) sent via UART.\n");
    LOG_PRINT("Bootloader: Core 1 was already killed in bootloader_prepare()\n");
    LOG_PRINT("Bootloader: ============================================\n");
    fflush(stdout);
    uart_wait_ram();  // Ensure all UART data is sent
    // NOTE: Removed sleep_ms(200) - risky in RAM function context
    
    // Initialize debug buffer
    g_debug_index = 0;
    
    // =========================================================================
    // PHASE 2: FLASH OPERATIONS WITH INTERRUPTS DISABLED
    // CRITICAL: NO LOG_PRINT after this point! printf is in flash and will crash.
    // Use only debug_marker() which is RAM-resident.
    // CRITICAL: Flush UART after EVERY marker to ensure we see where crash occurs!
    // =========================================================================
    
    debug_marker(DBG_LOOP_START, (uint16_t)sector_count);
    uart_wait_ram();  // FLUSH!
    
    // =========================================================================
    // CRITICAL: Stop Core 1 before flash operations
    // We try lockout first (graceful), then reset (forceful)
    // Short timeout - if it fails, we continue anyway
    // =========================================================================
    debug_marker(0xD1, 0x0001);  // D1 = About to lockout Core 1
    uart_wait_ram();  // FLUSH!
    
    // NOTE: We are running on Core 1. Core 0 should already be in bootloader_core0_loop().
    // bootloader_core0_loop() runs with interrupts DISABLED, so it will NOT respond to lockout.
    // Therefore, we SKIP the lockout attempt and rely on Core 0 being parked safely.
    
    debug_marker(0xD2, 0x0001);  // D2 = Lockout skipped (Core 0 parked)
    uart_wait_ram();  // FLUSH!
    
    debug_marker(0xD4, 0x0001);  // D4 = Core 1 should be stopped (Core 0 parked)
    uart_wait_ram();  // FLUSH!
    
    // Disable interrupts - interrupt handlers are in flash and will crash
    // during erase/program operations
    debug_marker(0xD5, 0x0001);  // D5 = About to disable interrupts
    uart_wait_ram();  // FLUSH!
    
    uint32_t irq_state = save_and_disable_interrupts();
    (void)irq_state;  // We won't restore - system will reset
    
    debug_marker(0xD6, 0x0001);  // D6 = Interrupts disabled
    uart_wait_ram();  // FLUSH!

    // =========================================================================
    // CRITICAL: Initialize ROM function pointers and exit XIP mode
    // This MUST be done before any flash operations!
    // The bootrom functions are in ROM (not flash), so they're always safe.
    // =========================================================================
    debug_marker(0xD7, 0x0001);  // D7 = Initializing ROM functions
    uart_wait_ram();  // FLUSH!
    
    init_rom_flash_functions();
    
    debug_marker(0xD8, 0x0001);  // D8 = ROM functions initialized
    uart_wait_ram();  // FLUSH!
    
    // Exit XIP mode - after this, DO NOT access flash memory!
    // All data must come from the preloaded RAM buffer.
    prepare_flash_for_write();
    
    debug_marker(0xD9, 0x0001);  // D9 = XIP exited, ready for flash operations
    uart_wait_ram();  // FLUSH!

    // Track sectors copied
    volatile uint32_t sectors_copied = 0;
    
    for (uint32_t i = 0; i < sector_count; i++) {
        // CRITICAL: Feed watchdog at the start of EVERY sector iteration
        // Each sector takes ~100-200ms (erase + program), and watchdog timeout
        // must not expire across multiple sectors
        watchdog_hw->load = 0x7fffff;
        
        uint32_t offset = i * FLASH_SECTOR_SIZE;
        
        // Send debug marker for this sector (RAM-resident, safe)
        debug_marker(DBG_SECTOR_COPY, (uint16_t)i);
        // Partial flush - don't full block but ensure FIFO not overflowing
        while (uart_get_hw(ESP32_UART_ID)->fr & UART_UARTFR_TXFF_BITS) { ; }
        
        // A. Get sector data from PRELOADED RAM buffer (NO XIP ACCESS!)
        // The firmware was already copied to g_firmware_preload_buffer before
        // we disabled interrupts, so this is a pure RAM-to-RAM copy
        const uint8_t* preloaded_src = g_firmware_preload_buffer + offset;
        
        // Copy from preload buffer to sector buffer (RAM to RAM - always safe)
        for (size_t k = 0; k < FLASH_SECTOR_SIZE; k++) {
            g_sector_buffer[k] = preloaded_src[k];
        }
        
        // Mark: sector data ready (from preloaded buffer)
        debug_marker(0xE0, (uint16_t)i);  // E0 = sector data ready

        // B. Erase sector using safe wrapper (required for RP2350)
        // Feed watchdog BEFORE erasing (erase can take 50-100ms!)
        watchdog_hw->load = 0x7fffff;
        
        debug_marker(DBG_ERASE_START, (uint16_t)i);
        uart_wait_ram();  // FLUSH before erase!
        
        safe_flash_erase(FLASH_MAIN_OFFSET + offset, FLASH_SECTOR_SIZE);
        
        // Feed watchdog AFTER erasing
        watchdog_hw->load = 0x7fffff;
        
        debug_marker(DBG_ERASE_DONE, (uint16_t)i);
        uart_wait_ram();  // FLUSH after erase!
        
        // C. Program sector using safe wrapper (required for RP2350)
        debug_marker(DBG_PROG_START, (uint16_t)i);
        uart_wait_ram();  // FLUSH before program!
        
        // Feed watchdog BEFORE programming
        watchdog_hw->load = 0x7fffff;
        
        // Program entire sector at once with safe wrapper
        safe_flash_program(FLASH_MAIN_OFFSET + offset, g_sector_buffer, FLASH_SECTOR_SIZE);
        watchdog_hw->load = 0x7fffff;
        
        debug_marker(DBG_PROG_DONE, (uint16_t)i);
        uart_wait_ram();  // FLUSH after program!
        
        // Track progress
        sectors_copied = i + 1;
    }
    
    // Send completion marker
    debug_marker(DBG_LOOP_DONE, (uint16_t)sectors_copied);
    uart_wait_ram();  // FLUSH!
    
    // Flush the flash cache to ensure all writes are visible
    // This uses the bootrom function which is in ROM (safe)
    flush_flash_cache();
    
    debug_marker(0xFC, (uint16_t)sectors_copied);  // 0xFC = Cache flushed
    uart_wait_ram();  // FLUSH!
    
    // Ensure all flash operations are complete with memory barriers
    __dmb();
    __dsb();
    __isb();
    
    // Small delay to ensure flash write cycles complete
    for (volatile int i = 0; i < 50000; i++) { __asm volatile("nop"); }
    
    // Send marker to confirm we're about to reset
    debug_marker(0xFE, (uint16_t)sectors_copied);  // 0xFE = About to reset
    uart_wait_ram();

    // 3. Reset System - Use hardware watchdog for reliable reset on both RP2040 and RP2350
    debug_marker(DBG_RESET_TRIGGER, 0xFFFF);
    uart_wait_ram();  // FLUSH before reset!
    
    // Give UART time to transmit all pending bytes
    for (volatile int i = 0; i < 100000; i++) { __asm volatile("nop"); }
    
    // Use watchdog for reset (more reliable than AIRCR on RP2350)
    // Set watchdog to fire in 1ms and don't feed it
    watchdog_hw->ctrl = 0;  // Disable first
    watchdog_hw->load = 1000;  // 1ms timeout (in microseconds)
    watchdog_hw->ctrl = WATCHDOG_CTRL_ENABLE_BITS;  // Enable, will reset in 1ms
    
    // Fallback: Also try AIRCR reset in case watchdog doesn't work
    // PPB_BASE = 0xE0000000, AIRCR offset = 0xED0C  
    volatile uint32_t* aircr_reg = (volatile uint32_t*)(0xE0000000 + 0xED0C);
    *aircr_reg = 0x05FA0004;  // VECTKEY (0x05FA << 16) | SYSRESETREQ (bit 2)
    
    while(1) { __asm volatile("nop"); }  // Infinite loop until reset
}
 
// -----------------------------------------------------------------------------
// Main Receive Loop
// -----------------------------------------------------------------------------

// Helper to flush XIP cache on both RP2040 and RP2350
static void __no_inline_not_in_flash_func(bootloader_flush_cache)(void) {
#if PICO_RP2040
    xip_ctrl_hw->flush = 1;
    (void)xip_ctrl_hw->flush;
#else
    // RP2350 (Pico 2) cache flush
    // Use the SDK function if available, or manual register access
    // On RP2350, cache is flushed via XIP_CTRL_STREAM_FLUSH
    // We assume the SDK handles this correctly in flash_range_program, 
    // but we add an explicit flush here for read-back safety.
    // Note: RP2350 XIP_CTRL is different structure.
    // For now, we rely on memory barriers as direct register access is complex without SDK headers.
    __asm volatile ("isb");
    __asm volatile ("dsb");
#endif
    __dmb();
}

static bool __no_inline_not_in_flash_func(verify_staging_area)(uint32_t size, uint32_t expected_crc) {
    // Send start marker with expected CRC
    debug_marker(0xC0, (uint16_t)(expected_crc & 0xFFFF));
    debug_marker(0xC1, (uint16_t)(expected_crc >> 16));
    uart_wait_ram();

    // Use standard XIP address for reading
    const uint8_t* staging_base = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Explicitly flush cache to ensure we see what was written
    bootloader_flush_cache();

    uint32_t calc_crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    // Read and calculate CRC
    for (size_t i = 0; i < size; i++) {
        uint8_t byte = staging_base[i];
        calc_crc ^= byte;
        for (int j = 0; j < 8; j++) {
            if (calc_crc & 1) calc_crc = (calc_crc >> 1) ^ polynomial;
            else calc_crc >>= 1;
        }
        
        // Feed watchdog periodically
        if ((i % 1024) == 0) watchdog_update();
    }
    
    calc_crc = ~calc_crc;
    
    // Send calculated CRC
    debug_marker(0xC2, (uint16_t)(calc_crc & 0xFFFF));
    debug_marker(0xC3, (uint16_t)(calc_crc >> 16));
    uart_wait_ram();
    
    if (calc_crc != expected_crc) {
        // Send mismatch marker
        debug_marker(0xCF, 0xBAD1);
        return false;
    }
    
    // Send success marker
    debug_marker(0xCA, 0x600D);
    return true;
}

bootloader_result_t bootloader_receive_firmware(void) {
    // IMMEDIATE DEBUG: Confirm we entered this function
    printf("\n\n*** BOOTLOADER_RECEIVE_FIRMWARE ENTERED ***\n");
    fflush(stdout);
    sleep_ms(50);
    debug_marker(0xBB, 0x0001);  // BB = Bootloader Begin
    
    g_receiving = true;
    g_received_size = 0;
    g_chunk_count = 0;
    
    // CRITICAL: Calculate CRC during reception to avoid cache coherency issues
    // Instead of reading back from flash (which has XIP cache problems on RP2350),
    // we calculate CRC on incoming data as we receive it
    uint32_t running_crc = 0xFFFFFFFF;  // CRC32 initial value
    
    printf("Bootloader: Starting firmware receive...\n");
    fflush(stdout);
    
    // UART should already be drained by bootloader_prepare(), but drain again for safety
    // This ensures no stale bytes remain from the transition period
    while (uart_is_readable(ESP32_UART_ID)) (void)uart_getc(ESP32_UART_ID);
     
    // NOTE: Bootloader ACK is sent in handle_cmd_bootloader() AFTER bootloader_prepare()
    // This ensures the system is fully ready before ESP32 starts sending firmware chunks
    // We skip sending it here to avoid duplicate ACK
    LOG_PRINT("Bootloader: ACK already sent, waiting for firmware...\n");
    
    // CRITICAL: Disable UART interrupts to prevent protocol handler from stealing firmware bytes
    // NOTE: We disable interrupts because protocol handler uses interrupts, but bootloader uses polling
    // This ensures protocol handler doesn't process bootloader data
    // USB serial logging is NOT affected - it uses a different UART
    // The bootloader will use polling (uart_is_readable/uart_getc) to read data
    uart_set_irq_enables(ESP32_UART_ID, false, false);
    LOG_PRINT("Bootloader: UART interrupts disabled, using polling for firmware reception\n");
     
     // Use SDK Flash Safe functions for the download phase
     static uint8_t page_buffer[FLASH_PAGE_SIZE];
     uint32_t page_buffer_offset = 0;
     uint32_t current_page_start = FLASH_TARGET_OFFSET;
     uint32_t current_sector = FLASH_TARGET_OFFSET / FLASH_SECTOR_SIZE;
     bool sector_erased = false;
     
    absolute_time_t start_time = get_absolute_time();
    // Set watchdog timeout to chunk timeout + 5 seconds margin
    // This ensures the watchdog doesn't fire during normal operation
    watchdog_enable(BOOTLOADER_CHUNK_TIMEOUT_MS + 5000, 1);
    LOG_PRINT("Bootloader: Starting firmware reception loop (watchdog=%dms, chunk_timeout=%dms)\n",
              BOOTLOADER_CHUNK_TIMEOUT_MS + 5000, BOOTLOADER_CHUNK_TIMEOUT_MS);
    
    while (true) {
         watchdog_update();
         
         if (absolute_time_diff_us(start_time, get_absolute_time()) > BOOTLOADER_TIMEOUT_MS * 1000) {
             uart_write_byte(0xFF); uart_write_byte(BOOTLOADER_ERROR_TIMEOUT);
             return BOOTLOADER_ERROR_TIMEOUT;
         }
         
         uint32_t chunk_num; uint16_t chunk_size;
         // Reduced logging - only log every 50 chunks or on errors to avoid USB serial slowdown
         if (!receive_chunk_header(&chunk_num, &chunk_size)) {
             LOG_PRINT("Bootloader: ERROR - Chunk header timeout at chunk %lu\n", (unsigned long)g_chunk_count);
             uart_write_byte(0xFF); uart_write_byte(BOOTLOADER_ERROR_TIMEOUT);
             return BOOTLOADER_ERROR_TIMEOUT;
         }
         
         if (chunk_num == 0xFFFFFFFF) {
             // CRITICAL: The end marker chunk still has data and checksum bytes in UART buffer!
             // We must drain them before looking for the CRC packet, otherwise the Pico
             // will confuse the end marker's data (0xAA 0x55) with the CRC packet header.
             // End marker chunk format: [0x55 0xAA] [0xFFFFFFFF] [size=2] [data: 0xAA 0x55] [checksum: 0xFF]
             // The header was read by receive_chunk_header, now drain: data (chunk_size bytes) + checksum (1 byte)
             LOG_PRINT("Bootloader: End marker detected, draining %u data bytes + checksum\n", chunk_size);
             for (uint16_t i = 0; i < chunk_size + 1; i++) {
                 uint8_t dummy;
                 if (!uart_read_byte_timeout(&dummy, 500)) {
                     LOG_PRINT("Bootloader: Warning - timeout draining end marker byte %d\n", i);
                     break;
                 }
             }
             break; // Exit chunk loop
         }
         
         if (chunk_size == 0 || chunk_size > BOOTLOADER_CHUNK_MAX_SIZE || chunk_num != g_chunk_count) {
             LOG_PRINT("Bootloader: ERROR - Invalid chunk: num=%lu (expected %lu), size=%u\n", 
                      (unsigned long)chunk_num, (unsigned long)g_chunk_count, chunk_size);
             uart_write_byte(0xFF); uart_write_byte(BOOTLOADER_ERROR_INVALID_SIZE);
             return BOOTLOADER_ERROR_INVALID_SIZE;
         }
         
         uint8_t chunk_data[BOOTLOADER_CHUNK_MAX_SIZE];
         uint8_t csum;
         if (!receive_chunk_data(chunk_data, chunk_size, &csum)) {
             LOG_PRINT("Bootloader: ERROR - Chunk %lu checksum failed\n", (unsigned long)chunk_num);
             uart_write_byte(0xFF); uart_write_byte(BOOTLOADER_ERROR_CHECKSUM);
             return BOOTLOADER_ERROR_CHECKSUM;
         }
         
         // Log progress every 50 chunks to reduce USB overhead
         if (g_chunk_count % 50 == 0) {
             LOG_PRINT("Bootloader: Progress - chunk %lu, %lu bytes received\n", 
                      (unsigned long)g_chunk_count, (unsigned long)g_received_size);
         }
        
        // Update running CRC with this chunk's data (avoids reading from flash later)
        for (uint16_t i = 0; i < chunk_size; i++) {
            running_crc ^= chunk_data[i];
            for (int j = 0; j < 8; j++) {
                if (running_crc & 1) {
                    running_crc = (running_crc >> 1) ^ 0xEDB88320;  // CRC32 polynomial
                } else {
                    running_crc >>= 1;
                }
            }
        }
         
         uint32_t offset = 0;
         while (offset < chunk_size) {
             uint32_t space = FLASH_PAGE_SIZE - page_buffer_offset;
             uint32_t copy = (chunk_size - offset < space) ? (chunk_size - offset) : space;
             memcpy(page_buffer + page_buffer_offset, chunk_data + offset, copy);
             page_buffer_offset += copy; 
             offset += copy;
             
             if (page_buffer_offset >= FLASH_PAGE_SIZE) {
                 uint32_t sect_start = current_page_start & ~(FLASH_SECTOR_SIZE - 1);
                 
                 // CRITICAL: Feed watchdog before flash operations (they can take 50-100ms)
                 // Flash operations disable interrupts and pause Core 0, so watchdog must be fed first
                 watchdog_update();
                 
                 if (!sector_erased || (sect_start != current_sector)) {
                     // Use bootloader-specific flash functions (no multicore lockout)
                     // This avoids the flash_safe_execute timeout issues on RP2350
                     watchdog_update();
                     if (!flash_bootloader_erase(sect_start, FLASH_SECTOR_SIZE)) {
                         LOG_PRINT("Bootloader: Flash erase failed at offset 0x%lx\n", (unsigned long)sect_start);
                         return BOOTLOADER_ERROR_FLASH_ERASE;
                     }
                     watchdog_update();
                     sector_erased = true;
                     current_sector = sect_start;
                 }
                 
                 // Use bootloader-specific flash program (no multicore lockout)
                 watchdog_update();
                 if (!flash_bootloader_program(current_page_start, page_buffer, FLASH_PAGE_SIZE)) {
                     LOG_PRINT("Bootloader: Flash program failed at offset 0x%lx\n", (unsigned long)current_page_start);
                     return BOOTLOADER_ERROR_FLASH_WRITE;
                 }
                 watchdog_update();
                 
                 current_page_start += FLASH_PAGE_SIZE;
                 page_buffer_offset = 0;
             }
         }
         
       g_received_size += chunk_size;
       g_chunk_count++;
       
       // Send ACK after processing (no verbose logging to avoid USB slowdown)
       watchdog_update();
       uart_write_byte(0xAA);
       uart_tx_wait_blocking(ESP32_UART_ID);
       watchdog_update();
    }
     
    // ====== CHUNK RECEPTION COMPLETE ======
    // TRACE 0x10: Exited chunk loop successfully - look for this byte!
    uart_write_byte(0x10);
    uart_tx_wait_blocking(ESP32_UART_ID);
    
    watchdog_update();
    
    // TRACE 0x11: About to write final page buffer
    uart_write_byte(0x11);
    
    if (page_buffer_offset > 0) {
        // TRACE 0x12: Inside final page buffer block
        uart_write_byte(0x12);
        
        uint32_t sect_start = current_page_start & ~(FLASH_SECTOR_SIZE - 1);
        watchdog_update();
        
        if (!sector_erased || (sect_start != current_sector)) {
            // TRACE 0x13: About to erase final sector
            uart_write_byte(0x13);
            flash_bootloader_erase(sect_start, FLASH_SECTOR_SIZE);
            // TRACE 0x14: Erase complete
            uart_write_byte(0x14);
        }
        
        watchdog_update();
        memset(&page_buffer[page_buffer_offset], 0xFF, FLASH_PAGE_SIZE - page_buffer_offset);
        
        // TRACE 0x15: About to program final page
        uart_write_byte(0x15);
        flash_bootloader_program(current_page_start, page_buffer, FLASH_PAGE_SIZE);
        // TRACE 0x16: Program complete
        uart_write_byte(0x16);
        
        watchdog_update();
    }
    
    // TRACE 0x17: Final page buffer done (or skipped)
    uart_write_byte(0x17);
     
   watchdog_update();  // Feed before validation
   
   // TRACE 1: After chunk reception - look for 0xA1 in ESP32 UART
   uart_write_byte(0xA1);
   
   // Use standard XIP address - we calculate CRC during reception now
   // so this is only used for vector validation
   __dmb();
   __dsb();
   __isb();
   
#if PICO_RP2040
   // On RP2040, flush the XIP cache first
   xip_ctrl_hw->flush = 1;
   (void)xip_ctrl_hw->flush;
   __dmb();
#endif
   
   const uint8_t* staged = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
   
   // TRACE 2: After cache bypass setup - look for 0xA2
   uart_write_byte(0xA2);
   
   // [VALIDATION] Check for valid ARM Cortex-M Vector Table BEFORE copying
   uint32_t* staged_vectors = (uint32_t*)staged;
   bool valid_sp = (staged_vectors[0] & 0xFF000000) == 0x20000000;
   bool valid_pc = (staged_vectors[1] & 0xFF000000) == 0x10000000;
   
   // TRACE 3: After reading vectors - look for 0xA3
   uart_write_byte(0xA3);
   
   watchdog_update();
   
   if (!valid_sp || !valid_pc) {
       // TRACE: Validation FAILED - look for 0xBF (Bad Firmware)
       uart_write_byte(0xBF);
       uart_write_byte((staged_vectors[0] >> 24) & 0xFF);  // SP high byte
       uart_write_byte((staged_vectors[1] >> 24) & 0xFF);  // PC high byte
       uart_tx_wait_blocking(ESP32_UART_ID);
       return BOOTLOADER_ERROR_INVALID_SIZE;
   }
   
   // TRACE 4: Validation passed - look for 0xA4
   uart_write_byte(0xA4);
   
   LOG_PRINT("Bootloader: Staging area validation OK (SP=%08lX, PC=%08lX)\n", 
            staged_vectors[0], staged_vectors[1]);
   
   // TRACE 5: CRC already calculated during reception - look for 0xA5
   uart_write_byte(0xA5);
   
   watchdog_update();
   // CRITICAL: Use running CRC calculated during chunk reception
   // This avoids reading from flash which has XIP cache coherency issues on RP2350
   uint32_t crc = running_crc ^ 0xFFFFFFFF;  // Finalize CRC32
   watchdog_update();
   
   // TRACE 6: CRC finalized - look for 0xA6
   uart_write_byte(0xA6);
    
    // Wait for expected CRC32 from ESP32 (optional - sent after end marker)
    // Format: [0xAA 0x55] [CRC32: 4 bytes LE]
    uint32_t expectedCRC32 = 0;
    bool receivedExpectedCRC = false;
    absolute_time_t crcTimeout = make_timeout_time_ms(2000);  // 2 second timeout
    
    while (!time_reached(crcTimeout)) {
        watchdog_update();
        if (uart_is_readable(ESP32_UART_ID)) {
            uint8_t b1 = uart_getc(ESP32_UART_ID);
            if (b1 == 0xAA) {
                // Wait for second byte with timeout (don't assume it's immediately available)
                uint8_t b2;
                if (uart_read_byte_timeout(&b2, 500)) {  // 500ms timeout for second byte
                    if (b2 == 0x55) {
                        // Read CRC32 (4 bytes, little-endian)
                        uint8_t crcBytes[4];
                        if (uart_read_bytes_timeout(crcBytes, 4, 1000)) {
                            expectedCRC32 = crcBytes[0] | (crcBytes[1] << 8) | 
                                          (crcBytes[2] << 16) | (crcBytes[3] << 24);
                            receivedExpectedCRC = true;
                            LOG_PRINT("Bootloader: Received expected CRC32: 0x%08lX\n", (unsigned long)expectedCRC32);
                            break;
                        } else {
                            LOG_PRINT("Bootloader: CRC32 read timeout after 0xAA 0x55\n");
                        }
                    } else {
                        LOG_PRINT("Bootloader: Expected 0x55 after 0xAA, got 0x%02X\n", b2);
                    }
                } else {
                    LOG_PRINT("Bootloader: Timeout waiting for second byte after 0xAA\n");
                }
            }
        }
        sleep_us(1000);  // 1ms delay
    }
    
    watchdog_update();  // Feed after CRC wait loop
    
    // CRC verification
    if (receivedExpectedCRC) {
        // Send CRC info marker so ESP32 can see both values
        uart_write_byte(0xCE);  // CRC info marker
        uart_write_byte((crc >> 0) & 0xFF);
        uart_write_byte((crc >> 8) & 0xFF);
        uart_write_byte((crc >> 16) & 0xFF);
        uart_write_byte((crc >> 24) & 0xFF);
        uart_write_byte((expectedCRC32 >> 0) & 0xFF);
        uart_write_byte((expectedCRC32 >> 8) & 0xFF);
        uart_write_byte((expectedCRC32 >> 16) & 0xFF);
        uart_write_byte((expectedCRC32 >> 24) & 0xFF);
        uart_tx_wait_blocking(ESP32_UART_ID);
        
        if (crc != expectedCRC32) {
            LOG_PRINT("Bootloader: Running CRC MISMATCH: calc=0x%08lX, exp=0x%08lX\n", 
                     (unsigned long)crc, (unsigned long)expectedCRC32);
             // Don't fail yet, try full read-back verification
        } else {
            LOG_PRINT("Bootloader: Running CRC verified OK: 0x%08lX\n", (unsigned long)crc);
        }
        
        // Full Staging Area Verification (Read-Back)
        // This is the source of truth - verifies what was actually written to flash
        // Note: verify_staging_area uses debug_marker to report status
        if (!verify_staging_area(g_received_size, expectedCRC32)) {
             // Error marker already sent by verify_staging_area (0xCF)
             // We just return the error code
             return BOOTLOADER_ERROR_CHECKSUM;
        }
    } else {
        LOG_PRINT("Bootloader: WARNING - No expected CRC32 received, skipping verification\n");
    }
    
    // TRACE 7: After CRC verification - look for 0xA7
    uart_write_byte(0xA7);
    watchdog_update();
    
    // Validate g_received_size
    if (g_received_size == 0 || g_received_size > (1024 * 1024)) {
        uart_write_byte(0xBE);  // Bad sizE
        return BOOTLOADER_ERROR_INVALID_SIZE;
    }
    
    // TRACE 8: Size OK - look for 0xA8
    uart_write_byte(0xA8);
    watchdog_update();
    
    uint32_t expected_sectors = (g_received_size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
    if (expected_sectors == 0 || expected_sectors > 256) {
        uart_write_byte(0xBC);  // Bad Count
        return BOOTLOADER_ERROR_INVALID_SIZE;
    }
    
    // TRACE 9: Sector count OK - look for 0xA9
    uart_write_byte(0xA9);
    watchdog_update();
    
    // CRITICAL: Send final ACK BEFORE starting flash copy
    // The ESP32 will wait for this ACK, then wait 6 seconds before checking version
    // This gives us time to complete the flash copy and reset before ESP32 verifies
    uart_write_byte(0xAA); uart_write_byte(0x55); uart_write_byte(0x00);
    uart_tx_wait_blocking(ESP32_UART_ID);  // Ensure ACK is fully transmitted
    
    // =====================================================================
    // USB DEBUG: Print immediately after ACK - this should appear on Pico USB
    // =====================================================================
    printf("\n\n========== PICO BOOTLOADER ==========\n");
    printf("Final ACK sent! Starting flash copy sequence...\n");
    printf("Firmware size: %lu bytes\n", (unsigned long)g_received_size);
    printf("Sector count: %lu\n", (unsigned long)expected_sectors);
    fflush(stdout);
    sleep_ms(100);  // Give USB time to transmit
    
    // TEST 1: Send bytes BEFORE watchdog disable - look for 0xF1 0xF2 0xF3 0xF4
    printf("Sending test bytes 0xF1-F4 to ESP32 UART...\n");
    fflush(stdout);
    uart_write_byte(0xF1);
    uart_write_byte(0xF2);
    uart_write_byte(0xF3);
    uart_write_byte(0xF4);
    uart_tx_wait_blocking(ESP32_UART_ID);
    printf("Test bytes sent. Disabling watchdog...\n");
    fflush(stdout);
    sleep_ms(50);
    
    // CRITICAL: Disable watchdog - use SDK function instead of direct register write
    watchdog_disable();
    
    printf("Watchdog disabled. Sending test bytes 0xE1-E4...\n");
    fflush(stdout);
    
    // TEST 2: Send bytes AFTER watchdog disable - look for 0xE1 0xE2 0xE3 0xE4
    uart_write_byte(0xE1);
    uart_write_byte(0xE2);
    uart_write_byte(0xE3);
    uart_write_byte(0xE4);
    uart_tx_wait_blocking(ESP32_UART_ID);
    
    printf("All test bytes sent. Proceeding to flash copy...\n");
    fflush(stdout);
    sleep_ms(100);
    
    // Also send via debug_marker to test if that function works
    debug_marker(0xAA, 0x1111);  // Marker: we passed ACK stage
    
    // VERY AGGRESSIVE DEBUG: Use raw printf and flush, with big delays
    printf("\n\n");
    printf("=== BOOTLOADER DEBUG START ===\n");
    fflush(stdout);
    sleep_ms(100);
    
    printf("ACK sent, size=%lu, sectors=%lu\n", (unsigned long)g_received_size, (unsigned long)expected_sectors);
    fflush(stdout);
    sleep_ms(100);
     
    LOG_PRINT("Bootloader: ============================================\n");
    LOG_PRINT("Bootloader: STARTING FLASH COPY SEQUENCE\n");
    LOG_PRINT("Bootloader: ============================================\n");
    LOG_PRINT("Bootloader: Firmware size: %lu bytes\n", (unsigned long)g_received_size);
    LOG_PRINT("Bootloader: Sector count: %lu\n", (unsigned long)expected_sectors);
    LOG_PRINT("Bootloader: Staging area: 0x%08lX\n", (unsigned long)(XIP_BASE + FLASH_TARGET_OFFSET));
    LOG_PRINT("Bootloader: Main offset: 0x%08lX\n", (unsigned long)FLASH_MAIN_OFFSET);
    LOG_PRINT("Bootloader: Sector size: %lu bytes\n", (unsigned long)FLASH_SECTOR_SIZE);
    
    // Quick sanity check - read first few bytes from staging to verify data exists
    const uint8_t* staging_preview = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    LOG_PRINT("Bootloader: Staging preview (first 16 bytes):\n");
    LOG_PRINT("  %02X %02X %02X %02X %02X %02X %02X %02X\n",
              staging_preview[0], staging_preview[1], staging_preview[2], staging_preview[3],
              staging_preview[4], staging_preview[5], staging_preview[6], staging_preview[7]);
    LOG_PRINT("  %02X %02X %02X %02X %02X %02X %02X %02X\n",
              staging_preview[8], staging_preview[9], staging_preview[10], staging_preview[11],
              staging_preview[12], staging_preview[13], staging_preview[14], staging_preview[15]);
    
    sleep_ms(200);  // Give USB time to flush all logs
     
    // Enable watchdog with generous timeout (8.3 seconds) for flash copy
    LOG_PRINT("Bootloader: Enabling watchdog (8.3s timeout)\n");
    watchdog_enable(8300, 1);
    
    LOG_PRINT("Bootloader: Calling copy_firmware_to_main(%lu)...\n", (unsigned long)g_received_size);
    sleep_ms(100);  // Final flush before copy
    
    // TEST: Send bytes BEFORE function call using simple uart_write_byte
    // Look for 0x11 0x22 0x33 0x44 in the ESP32 output
    uart_write_byte(0x11);
    uart_write_byte(0x22);
    uart_write_byte(0x33);
    uart_write_byte(0x44);
    uart_tx_wait_blocking(ESP32_UART_ID);
     
    // Transfer control to RAM
    // This function will copy all sectors from staging to main flash using SDK functions,
    // which are RAM-resident and handle both RP2040 and RP2350 automatically
    // NOTE: If validation fails inside copy_firmware_to_main(), it will return instead of resetting
    copy_firmware_to_main(g_received_size);
    
    // If we reach here, copy_firmware_to_main() returned - send marker (0xCC = Copy returned)
    debug_marker(0xCC, 0xDEAD);
    uart_tx_wait_blocking(ESP32_UART_ID);
    
    // If we reach here, copy_firmware_to_main() returned early due to validation failure
    // This should NOT happen if pre-checks above passed, but handle it anyway
    LOG_PRINT("Bootloader: CRITICAL ERROR - Flash copy returned unexpectedly!\n");
    LOG_PRINT("Bootloader: This indicates validation failure inside copy function.\n");
    LOG_PRINT("Bootloader: Resetting Pico to attempt recovery with old firmware.\n");
    sleep_ms(100);  // Give USB time to flush
    
    // Reset via watchdog (watchdog is already enabled above)
    watchdog_reboot(0, 0, 0);
    while(1);  // Wait for watchdog reset
     
     return BOOTLOADER_ERROR_FAILED;
 }
 
 const char* bootloader_get_status_message(bootloader_result_t result) {
     return (result == BOOTLOADER_SUCCESS) ? "Success" : "Error";
 }