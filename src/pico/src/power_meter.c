/**
 * Power Meter Driver Implementation (Raspberry Pi Pico 2)
 * 
 * Hardware: PIO UART on GPIO6 (TX) and GPIO7 (RX), GPIO20 for RS485 DE/RE
 */

#include "power_meter.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

#ifndef UNIT_TEST
// Real hardware environment
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "config_persistence.h"
#include "packet_handlers.h"  // For core1_signal_alive()
#include "pio_uart.h"         // PIO-based UART on GPIO6 (TX) / GPIO7 (RX)

#else
// Unit test environment
#include "pio_uart.h"

#endif

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================

// Pin mapping: GPIO6 = TX (data to meter), GPIO7 = RX (data from meter)
// Confirmed by schematic Sheet 8 signal path and PCB config.
// Note: RX path goes through JP3 jumper - must bridge pads 2-3 for TTL mode.
#define UART_TX_PIN_DEFAULT 6
#define UART_RX_PIN_DEFAULT 7
#define RS485_DE_RE_PIN 20
#define RS485_SWITCHING_DELAY_US 100  // Delay for RS485 transceiver switching

// Active TX/RX pins (may be swapped if wiring is reversed)
static uint8_t uart_tx_pin = UART_TX_PIN_DEFAULT;
static uint8_t uart_rx_pin = UART_RX_PIN_DEFAULT;
static bool pins_swapped = false;
static uint8_t last_configured_meter = 0xFF;  // Track meter type across disable/enable cycles

// Modbus protocol
#define MODBUS_FC_READ_HOLDING_REGS 0x03
#define MODBUS_FC_READ_INPUT_REGS   0x04

#define RESPONSE_TIMEOUT_MS 500
#define CONNECTION_TIMEOUT_MS 5000

// =============================================================================
// REGISTER MAPS FOR SUPPORTED METERS
// =============================================================================

static const modbus_register_map_t METER_MAPS[] = {
    // PZEM-004T V3
    {
        .name = "PZEM-004T V3",
        .slave_addr = 0xF8,
        .baud_rate = 9600,
        .is_rs485 = false,
        .voltage_reg = 0x0000,
        .voltage_scale = 0.1f,
        .current_reg = 0x0001,
        .current_scale = 0.001f,
        .power_reg = 0x0002,
        .power_scale = 1.0f,
        .energy_reg = 0x0003,
        .energy_scale = 1.0f,
        .energy_is_32bit = true,
        .frequency_reg = 0x0004,
        .frequency_scale = 0.1f,
        .pf_reg = 0x0005,
        .pf_scale = 0.01f,
        .function_code = MODBUS_FC_READ_INPUT_REGS,
        .num_registers = 10
    },
    // JSY-MK-163T
    {
        .name = "JSY-MK-163T",
        .slave_addr = 0x01,
        .baud_rate = 4800,
        .is_rs485 = false,
        .voltage_reg = 0x0048,
        .voltage_scale = 0.0001f,
        .current_reg = 0x0049,
        .current_scale = 0.0001f,
        .power_reg = 0x004A,
        .power_scale = 0.0001f,
        .energy_reg = 0x004B,
        .energy_scale = 0.001f,
        .energy_is_32bit = true,
        .frequency_reg = 0x0057,
        .frequency_scale = 0.01f,
        .pf_reg = 0x0056,
        .pf_scale = 0.001f,
        .function_code = MODBUS_FC_READ_HOLDING_REGS,
        .num_registers = 16
    },
    // JSY-MK-194T
    {
        .name = "JSY-MK-194T",
        .slave_addr = 0x01,
        .baud_rate = 4800,
        .is_rs485 = false,
        .voltage_reg = 0x0000,
        .voltage_scale = 0.01f,
        .current_reg = 0x0001,
        .current_scale = 0.01f,
        .power_reg = 0x0002,
        .power_scale = 0.1f,
        .energy_reg = 0x0003,
        .energy_scale = 0.01f,
        .energy_is_32bit = true,
        .frequency_reg = 0x0007,
        .frequency_scale = 0.01f,
        .pf_reg = 0x0008,
        .pf_scale = 0.001f,
        .function_code = MODBUS_FC_READ_HOLDING_REGS,
        .num_registers = 10
    },
    // Eastron SDM120
    {
        .name = "Eastron SDM120",
        .slave_addr = 0x01,
        .baud_rate = 2400,
        .is_rs485 = true,
        .voltage_reg = 0x0000,
        .voltage_scale = 1.0f,
        .current_reg = 0x0006,
        .current_scale = 1.0f,
        .power_reg = 0x000C,
        .power_scale = 1.0f,
        .energy_reg = 0x0048,
        .energy_scale = 1.0f,
        .energy_is_32bit = false,
        .frequency_reg = 0x0046,
        .frequency_scale = 1.0f,
        .pf_reg = 0x001E,
        .pf_scale = 1.0f,
        .function_code = MODBUS_FC_READ_INPUT_REGS,
        .num_registers = 2
    },
    // Eastron SDM230
    {
        .name = "Eastron SDM230",
        .slave_addr = 0x01,
        .baud_rate = 9600,
        .is_rs485 = true,
        .voltage_reg = 0x0000,
        .voltage_scale = 1.0f,
        .current_reg = 0x0006,
        .current_scale = 1.0f,
        .power_reg = 0x000C,
        .power_scale = 1.0f,
        .energy_reg = 0x0156,
        .energy_scale = 1.0f,
        .energy_is_32bit = false,
        .frequency_reg = 0x0046,
        .frequency_scale = 1.0f,
        .pf_reg = 0x001E,
        .pf_scale = 1.0f,
        .function_code = MODBUS_FC_READ_INPUT_REGS,
        .num_registers = 2
    }
};

#define METER_MAPS_COUNT (sizeof(METER_MAPS) / sizeof(METER_MAPS[0]))

// =============================================================================
// PRIVATE STATE
// =============================================================================

// Cross-core shared state: Core 1 writes (via power_meter_init from packet handler),
// Core 0 reads (via sensors_read / power_meter_is_initialized / power_meter_update).
// Flags checked frequently by the other core must be volatile to prevent the compiler
// from caching stale values in registers.
static volatile bool initialized = false;
static volatile bool has_ever_read = false;   // True after first successful Modbus read
static const modbus_register_map_t* current_map = NULL;
static power_meter_reading_t last_reading = {0};
static uint32_t last_success_time = 0;
static char last_error[64] = {0};
static power_meter_config_t current_config = {0};
static volatile bool g_pending_save = false;

// Number of consecutive Modbus failures before trying the next pin config
#define PIN_ROTATE_THRESHOLD 3
static volatile uint8_t consecutive_failures = 0;  // volatile: written by Core 0 (update) and Core 1 (init)

// =============================================================================
// MODBUS PROTOCOL HELPERS
// =============================================================================

static uint16_t modbus_crc16(const uint8_t* buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

static void set_rs485_direction(bool transmit) {
    if (current_map && current_map->is_rs485) {
        gpio_put(RS485_DE_RE_PIN, transmit);
        if (transmit) {
            sleep_us(RS485_SWITCHING_DELAY_US);
        }
    }
}

// Forward declarations for functions used by try_modbus_probe and configure_uart_pins
static bool send_modbus_request(uint8_t slave_addr, uint8_t function_code, 
                                uint16_t start_reg, uint16_t num_regs);
static bool receive_modbus_response(uint8_t* buffer, int max_len, int* bytes_read);
static bool verify_modbus_response(const uint8_t* buffer, int length);

/**
 * Configure PIO UART with the current TX/RX pin assignment.
 * Uses PIO instead of hardware UART1. PIO works on any GPIO pin.
 */
static void configure_uart_pins(uint32_t baud_rate) {
    pio_uart_reconfigure(uart_tx_pin, uart_rx_pin, baud_rate);
}

/**
 * Try a single Modbus read with the current UART pin configuration.
 * Returns true if a valid response was received.
 */
static bool try_modbus_probe(void) {
    if (!current_map) return false;
    
    // Clear any stale data
    while (pio_uart_is_readable()) {
        pio_uart_getc();
    }
    
    // Send request
    if (!send_modbus_request(current_map->slave_addr, current_map->function_code,
                            current_map->voltage_reg, current_map->num_registers)) {
        return false;
    }
    
    // Receive response
    uint8_t response_buffer[128];
    int bytes_read = 0;
    if (!receive_modbus_response(response_buffer, sizeof(response_buffer), &bytes_read)) {
        return false;
    }
    
    // Verify response is valid
    return verify_modbus_response(response_buffer, bytes_read);
}

static bool send_modbus_request(uint8_t slave_addr, uint8_t function_code, 
                                uint16_t start_reg, uint16_t num_regs) {
    uint8_t request[8];
    request[0] = slave_addr;
    request[1] = function_code;
    request[2] = (start_reg >> 8) & 0xFF;
    request[3] = start_reg & 0xFF;
    request[4] = (num_regs >> 8) & 0xFF;
    request[5] = num_regs & 0xFF;
    
    // Calculate CRC
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;
    
    // Send request
    set_rs485_direction(true);
    pio_uart_write_blocking(request, 8);
    set_rs485_direction(false);
    
    return true;
}

static bool receive_modbus_response(uint8_t* buffer, int max_len, int* bytes_read) {
    *bytes_read = 0;
    uint32_t start_time = time_us_32() / 1000;  // Convert to ms
    
    while ((time_us_32() / 1000 - start_time) < RESPONSE_TIMEOUT_MS) {
        if (pio_uart_is_readable()) {
            buffer[*bytes_read] = pio_uart_getc();
            (*bytes_read)++;
            
            // Check if we have enough bytes for a complete response
            if (*bytes_read >= 5) {
                uint8_t expected_len = buffer[2] + 5;  // data length + header + crc
                if (*bytes_read >= expected_len) {
                    return true;
                }
            }
            
            if (*bytes_read >= max_len) {
                break;
            }
            
            start_time = time_us_32() / 1000;  // Reset timeout on each byte
        }
        sleep_ms(1);
    }
    
    return false;
}

static bool verify_modbus_response(const uint8_t* buffer, int length) {
    if (length < 5) return false;
    
    // Check slave address
    if (buffer[0] != current_map->slave_addr) return false;
    
    // Check function code
    if (buffer[1] != current_map->function_code) return false;
    
    // Verify CRC
    uint16_t received_crc = buffer[length - 2] | (buffer[length - 1] << 8);
    uint16_t calculated_crc = modbus_crc16(buffer, length - 2);
    
    return received_crc == calculated_crc;
}

static uint16_t extract_uint16(const uint8_t* buffer, int offset) {
    return (buffer[offset] << 8) | buffer[offset + 1];
}

static uint32_t extract_uint32(const uint8_t* buffer, int offset) {
    return ((uint32_t)buffer[offset] << 24) | 
           ((uint32_t)buffer[offset + 1] << 16) |
           ((uint32_t)buffer[offset + 2] << 8) | 
           buffer[offset + 3];
}

static bool parse_response(const uint8_t* buffer, int length, power_meter_reading_t* reading) {
    if (length < 5) return false;
    
    uint8_t byte_count = buffer[2];
    const uint8_t* data = &buffer[3];
    
    // Initialize reading
    memset(reading, 0, sizeof(power_meter_reading_t));
    
    // Calculate register offsets from start (voltage is always first, so offset is 0)
    int voltage_offset = 0;
    int current_offset = (current_map->current_reg - current_map->voltage_reg) * 2;
    int power_offset = (current_map->power_reg - current_map->voltage_reg) * 2;
    int energy_offset = (current_map->energy_reg - current_map->voltage_reg) * 2;
    int frequency_offset = (current_map->frequency_reg - current_map->voltage_reg) * 2;
    int pf_offset = (current_map->pf_reg - current_map->voltage_reg) * 2;
    
    // Extract voltage
    if (voltage_offset >= 0 && voltage_offset + 1 < byte_count) {
        uint16_t raw = extract_uint16(data, voltage_offset);
        reading->voltage = raw * current_map->voltage_scale;
    }
    
    // Extract current
    if (current_offset >= 0 && current_offset + 1 < byte_count) {
        uint16_t raw = extract_uint16(data, current_offset);
        reading->current = raw * current_map->current_scale;
    }
    
    // Extract power
    if (power_offset >= 0 && power_offset + 1 < byte_count) {
        uint16_t raw = extract_uint16(data, power_offset);
        reading->power = raw * current_map->power_scale;
    }
    
    // Extract energy
    if (energy_offset >= 0 && energy_offset + (current_map->energy_is_32bit ? 3 : 1) < byte_count) {
        if (current_map->energy_is_32bit) {
            uint32_t raw = extract_uint32(data, energy_offset);
            reading->energy_import = raw * current_map->energy_scale / 1000.0f;  // Wh to kWh
        } else {
            uint16_t raw = extract_uint16(data, energy_offset);
            reading->energy_import = raw * current_map->energy_scale;
        }
    }
    
    // Extract frequency
    if (frequency_offset >= 0 && frequency_offset + 1 < byte_count) {
        uint16_t raw = extract_uint16(data, frequency_offset);
        reading->frequency = raw * current_map->frequency_scale;
    }
    
    // Extract power factor
    if (pf_offset >= 0 && pf_offset + 1 < byte_count) {
        uint16_t raw = extract_uint16(data, pf_offset);
        reading->power_factor = raw * current_map->pf_scale;
    }
    
    return true;
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

bool power_meter_init(const power_meter_config_t* config) {
    // CRITICAL: Always configure GPIO20 (RS485 DE/RE) as output LOW FIRST.
    // The MAX3485 transceiver's A/B outputs are permanently connected to J17 pins 4/5,
    // which are shared with the TTL signal path. If GPIO20 is left floating at boot,
    // DE may float HIGH, putting the MAX3485 in transmit mode. This causes the MAX3485
    // to actively drive J17 pins 4/5 with differential signals, overpowering any TTL
    // signals and making communication with TTL meters (PZEM, JSY) impossible.
    // This must happen BEFORE any early returns, including the disabled check.
    gpio_put(RS485_DE_RE_PIN, 0);   // Set LOW first (before enabling output to avoid glitch)
    gpio_init(RS485_DE_RE_PIN);
    gpio_set_dir(RS485_DE_RE_PIN, GPIO_OUT);
    gpio_put(RS485_DE_RE_PIN, 0);  // LOW = receive mode (A/B are high-impedance inputs)
    // Note: LOG_PRINT here would be lost - ring buffer not drained before log forwarding is active.
    // GPIO20 state is confirmed in power_meter_update's first-poll log instead.
    
    // Load or use provided config
    if (config) {
        // Runtime enable (from ESP32 command) - use provided config
        current_config = *config;
    } else {
        // Boot-time init: Power meter is always disabled at boot.
        // ESP32 re-sends the enable command after Pico connects (handleBoot).
        // No flash config is loaded - ESP32 is the sole source of truth.
        current_config.enabled = false;
        current_config.meter_index = 0xFF;
        current_config.slave_addr = 0;
        current_config.baud_rate = 0;
    }
    
    if (!current_config.enabled) {
        initialized = false;
        has_ever_read = false;
        current_map = NULL;
        return true;  // Disabled, nothing to do
    }
    
    /* Validate meter_index from flash to avoid out-of-bounds or long auto-detect at boot */
    if (current_config.meter_index != 0xFF && current_config.meter_index >= METER_MAPS_COUNT) {
        snprintf(last_error, sizeof(last_error), "Invalid meter index %u", (unsigned)current_config.meter_index);
        current_config.enabled = false;
        initialized = false;
        has_ever_read = false;
        current_map = NULL;
        return true;  /* Success but disabled */
    }
    
    // Select register map
    if (current_config.meter_index < METER_MAPS_COUNT) {
        current_map = &METER_MAPS[current_config.meter_index];
    } else if (current_config.meter_index == 0xFF) {
        /* Auto-detect: skip at boot when config is NULL (would block 10-15s, risk watchdog) */
        if (!config) {
            current_config.enabled = false;
            initialized = false;
            current_map = NULL;
            return true;  /* Boot: don't run auto_detect here; user can re-enable from UI */
        }
        return power_meter_auto_detect();
    } else {
        snprintf(last_error, sizeof(last_error), "Invalid meter index");
        return false;
    }
    
    // GPIO20 (DE/RE) already configured as output LOW at the top of this function.
    
    // Preserve pin swap state when re-initializing with the same meter type.
    // power_meter_init() is called from Core 1 (packet handler) and can race with
    // power_meter_update() on Core 0 which manages the pin swap/retry logic.
    // Only reset pin state when the meter type actually changes.
    bool meter_changed = (current_config.meter_index != last_configured_meter);
    last_configured_meter = current_config.meter_index;
    
    if (meter_changed) {
        // Different meter type (or first-ever init) - full reset of pin state
        uart_tx_pin = UART_TX_PIN_DEFAULT;
        uart_rx_pin = UART_RX_PIN_DEFAULT;
        pins_swapped = false;
        consecutive_failures = 0;
    }
    
    if (!initialized || meter_changed) {
        // First init, re-enable, or meter change: configure UART
        // Pin state is preserved for same meter re-enable (swap survives disable/enable)
        configure_uart_pins(current_map->baud_rate);
        LOG_PRINT("Power meter: Initialized (%s @ %u baud, RS485: %s, TX=GPIO%d, RX=GPIO%d%s)\n", 
                  current_map->name, (unsigned int)current_map->baud_rate,
                  current_map->is_rs485 ? "yes" : "no",
                  uart_tx_pin, uart_rx_pin,
                  pins_swapped ? " [swapped]" : "");
        if (meter_changed) {
            LOG_PRINT("Power meter: Will probe for meter on first update cycle\n");
        }
    } else {
        LOG_PRINT("Power meter: Already initialized (%s), keeping pin config (TX=GPIO%d, RX=GPIO%d, failures=%d)\n",
                  current_map->name, uart_tx_pin, uart_rx_pin, consecutive_failures);
    }
    
    initialized = true;
    return true;
}

void power_meter_update(void) {
    if (!initialized || !current_map) {
        return;
    }
    
    // Clear any stale data
    int stale_bytes = 0;
    while (pio_uart_is_readable()) {
        pio_uart_getc();
        stale_bytes++;
    }
    
    // Send Modbus request
    if (!send_modbus_request(current_map->slave_addr, current_map->function_code,
                            current_map->voltage_reg, current_map->num_registers)) {
        snprintf(last_error, sizeof(last_error), "Failed to send request");
        LOG_PRINT("Power meter: Failed to send Modbus request\n");
        return;
    }
    
    // Receive response
    uint8_t response_buffer[128];
    int bytes_read = 0;
    if (!receive_modbus_response(response_buffer, sizeof(response_buffer), &bytes_read)) {
        // Capture in local to prevent TOCTOU race with Core 1 resetting the volatile.
        uint8_t failures = ++consecutive_failures;
        
        // Log every failure in the swap cycle (1-6), then periodically
        if (failures <= PIN_ROTATE_THRESHOLD * 2 || failures % 10 == 0) {
            LOG_PRINT("Power meter: No response (attempt %d, TX=GPIO%d, RX=GPIO%d, DE=%d, stale=%d)\n",
                      failures, uart_tx_pin, uart_rx_pin, gpio_get(RS485_DE_RE_PIN), stale_bytes);
        }
        
        // After several failures, try swapping TX/RX pins (common wiring mistake)
        if (failures == PIN_ROTATE_THRESHOLD) {
            uint8_t old_tx = uart_tx_pin;
            uart_tx_pin = uart_rx_pin;
            uart_rx_pin = old_tx;
            pins_swapped = !pins_swapped;
            // Log BEFORE configure_uart_pins to ensure message is queued in ring buffer
            // before the PIO UART init message. Otherwise ring buffer overflow drops it.
            LOG_PRINT("Power meter: Swapping pins → TX=GPIO%d, RX=GPIO%d\n", uart_tx_pin, uart_rx_pin);
            configure_uart_pins(current_map->baud_rate);
            snprintf(last_error, sizeof(last_error), "No response - swapped TX/RX (TX=GPIO%d)", uart_tx_pin);
        } else if (failures == PIN_ROTATE_THRESHOLD * 2) {
            uint8_t old_tx = uart_tx_pin;
            uart_tx_pin = uart_rx_pin;
            uart_rx_pin = old_tx;
            pins_swapped = !pins_swapped;
            LOG_PRINT("Power meter: Reverting pins → TX=GPIO%d, RX=GPIO%d (cycle reset)\n", uart_tx_pin, uart_rx_pin);
            configure_uart_pins(current_map->baud_rate);
            consecutive_failures = 0;
            snprintf(last_error, sizeof(last_error), "No response - reverted TX/RX (TX=GPIO%d)", uart_tx_pin);
        } else {
            snprintf(last_error, sizeof(last_error), "No response from meter");
        }
        return;
    }
    
    // Got some bytes back - log for debugging
    if (!has_ever_read) {
        LOG_PRINT("Power meter: Got %d bytes response (TX=GPIO%d, RX=GPIO%d)\n",
                  bytes_read, uart_tx_pin, uart_rx_pin);
    }
    
    // Verify response
    if (!verify_modbus_response(response_buffer, bytes_read)) {
        snprintf(last_error, sizeof(last_error), "Invalid response (%d bytes, addr=0x%02X)", 
                 bytes_read, bytes_read > 0 ? response_buffer[0] : 0);
        LOG_PRINT("Power meter: Invalid response (%d bytes, first=0x%02X, expected addr=0x%02X)\n",
                  bytes_read, bytes_read > 0 ? response_buffer[0] : 0, current_map->slave_addr);
        return;
    }
    
    // Parse response
    power_meter_reading_t reading;
    if (!parse_response(response_buffer, bytes_read, &reading)) {
        snprintf(last_error, sizeof(last_error), "Parse error");
        LOG_PRINT("Power meter: Parse error (%d bytes)\n", bytes_read);
        return;
    }
    
    // Success - reset failure counter
    if (!has_ever_read || consecutive_failures > 0) {
        LOG_PRINT("Power meter: Connected! %.1fV %.2fA %.1fW (TX=GPIO%d, RX=GPIO%d)\n",
                  reading.voltage, reading.current, reading.power,
                  uart_tx_pin, uart_rx_pin);
    }
    consecutive_failures = 0;
    reading.timestamp = time_us_32() / 1000;
    reading.valid = true;
    last_reading = reading;
    last_success_time = reading.timestamp;
    has_ever_read = true;
    last_error[0] = '\0';
}

bool power_meter_get_reading(power_meter_reading_t* reading) {
    if (!reading) return false;
    
    // Check if reading is fresh (within last 5 seconds)
    uint32_t now = time_us_32() / 1000;
    if (last_reading.valid && (now - last_success_time) < CONNECTION_TIMEOUT_MS) {
        *reading = last_reading;
        return true;
    }
    
    return false;
}

bool power_meter_is_connected(void) {
    if (!initialized) return false;
    
    // Don't report connected until at least one successful read has occurred.
    // Without this, last_success_time=0 causes a false positive for the first
    // 5 seconds after boot, triggering blocking 500ms Modbus polls at 20Hz
    // which can exceed the 2000ms watchdog timeout during boot.
    if (!has_ever_read) return false;
    
    uint32_t now = time_us_32() / 1000;
    return (now - last_success_time) < CONNECTION_TIMEOUT_MS;
}

bool power_meter_is_initialized(void) {
    return initialized;
}

const char* power_meter_get_name(void) {
    return current_map ? current_map->name : "None";
}

bool power_meter_auto_detect(void) {
    printf("Starting power meter auto-detection (both pin configs)...\n");
    
    // CRITICAL: Ensure GPIO20 (DE/RE) is output LOW before probing ANY meter.
    // The MAX3485 A/B outputs share J17 pins 4/5 with the TTL signal path.
    // If DE is floating/HIGH, the transceiver drives those pins and blocks TTL meters.
    gpio_init(RS485_DE_RE_PIN);
    gpio_set_dir(RS485_DE_RE_PIN, GPIO_OUT);
    gpio_put(RS485_DE_RE_PIN, 0);
    
    // Try each known meter configuration, with both pin orientations
    for (uint8_t i = 0; i < METER_MAPS_COUNT; i++) {
        const modbus_register_map_t* test_map = &METER_MAPS[i];
        current_map = test_map;
        
        // Try both pin configurations: default and swapped
        for (uint8_t pin_try = 0; pin_try < 2; pin_try++) {
            if (pin_try == 0) {
                uart_tx_pin = UART_TX_PIN_DEFAULT;
                uart_rx_pin = UART_RX_PIN_DEFAULT;
                pins_swapped = false;
            } else {
                uart_tx_pin = UART_RX_PIN_DEFAULT;
                uart_rx_pin = UART_TX_PIN_DEFAULT;
                pins_swapped = true;
            }
            
            printf("Trying %s @ %u baud (TX=GPIO%d, RX=GPIO%d)...\n", 
                   test_map->name, (unsigned int)test_map->baud_rate,
                   uart_tx_pin, uart_rx_pin);
            
            // CRITICAL: Signal Core 1 is alive before each attempt.
            // Auto-detect runs on Core 1 and can block for ~600ms per attempt.
#ifndef UNIT_TEST
            core1_signal_alive();
#endif
            
            configure_uart_pins(test_map->baud_rate);
            sleep_ms(50);  // Let UART settle
            
            // Try to read
            power_meter_reading_t test_reading;
            
            // Clear buffer
            while (pio_uart_is_readable()) {
                pio_uart_getc();
            }
            
            // Send request
            if (!send_modbus_request(test_map->slave_addr, test_map->function_code,
                                    test_map->voltage_reg, test_map->num_registers)) {
                continue;
            }
            
            // Receive response (blocks up to RESPONSE_TIMEOUT_MS = 500ms)
            uint8_t response_buffer[128];
            int bytes_read = 0;
            if (!receive_modbus_response(response_buffer, sizeof(response_buffer), &bytes_read)) {
#ifndef UNIT_TEST
                core1_signal_alive();
#endif
                continue;
            }
            
            // Verify and parse
            if (verify_modbus_response(response_buffer, bytes_read) &&
                parse_response(response_buffer, bytes_read, &test_reading)) {
                
                // Verify reading is reasonable
                if (test_reading.voltage > 50 && test_reading.voltage < 300) {
                    printf("Detected: %s on %s pins (TX=GPIO%d, RX=GPIO%d)\n", 
                           test_map->name, pins_swapped ? "swapped" : "default",
                           uart_tx_pin, uart_rx_pin);
                    initialized = true;
                    has_ever_read = true;
                    last_reading = test_reading;
                    last_success_time = time_us_32() / 1000;
                    
                    current_config.enabled = true;
                    current_config.meter_index = i;
                    last_configured_meter = i;  // Track for re-init preservation
                    // Note: config NOT saved to Pico flash (ESP32 handles persistence)
                    
                    return true;
                }
            }
            
            sleep_ms(100);  // Wait before next attempt
        }
    }
    
    printf("No power meter detected on either pin configuration\n");
    snprintf(last_error, sizeof(last_error), "Auto-detection failed");
    initialized = false;
    current_map = NULL;
    uart_tx_pin = UART_TX_PIN_DEFAULT;
    uart_rx_pin = UART_RX_PIN_DEFAULT;
    pins_swapped = false;
    return false;
}

bool power_meter_save_config(void) {
#ifndef UNIT_TEST
    // Save current config to flash using config_persistence system
    power_meter_config_t cfg = {
        .enabled = initialized,
        .meter_index = current_map ? (uint8_t)(current_map - METER_MAPS) : 0xFF,
        .slave_addr = current_map ? current_map->slave_addr : 0,
        .baud_rate = current_map ? current_map->baud_rate : 0
    };
    return config_persistence_save_power_meter(&cfg);
#else
    // In unit tests, always succeed
    return true;
#endif
}

void power_meter_request_save(void) {
    g_pending_save = true;
}

bool power_meter_process_pending_save(void) {
    if (!g_pending_save) {
        return false;
    }
    g_pending_save = false;
    return power_meter_save_config();
}

bool power_meter_load_config(power_meter_config_t* config) {
#ifndef UNIT_TEST
    if (!config) {
        return false;
    }
    // Load from flash using config_persistence system
    config_persistence_get_power_meter(config);
    
    // Validate config: enabled flag AND valid meter index (not 0xFF and within bounds)
    // This ensures we only return true for configs that can actually be used at boot
    if (config->enabled && 
        config->meter_index != 0xFF && 
        config->meter_index < METER_MAPS_COUNT) {
        return true;
    }
    
    // Invalid config - reset to safe defaults to prevent boot issues
    if (config->enabled) {
        // Config was enabled but invalid - disable it to prevent problems
        config->enabled = false;
        config->meter_index = 0xFF;
    }
    return false;
#else
    // In unit tests, always fail (no config)
    (void)config;
    return false;
#endif
}

const char* power_meter_get_error(void) {
    return last_error[0] != '\0' ? last_error : NULL;
}

