/**
 * Pico Firmware - Hardware Abstraction Layer
 * 
 * Provides a clean interface to hardware peripherals (ADC, SPI, PWM, GPIO)
 * with support for simulation mode for development and testing.
 * 
 * Usage:
 *   - Call hw_init() during system initialization
 *   - Use hw_set_simulation_mode() to enable/disable simulation
 *   - All hardware functions work the same in simulation and real mode
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Configuration
// =============================================================================

// Enable simulation mode by default for development
// Set to 0 to use real hardware
#ifndef HW_SIMULATION_MODE
#define HW_SIMULATION_MODE 1
#endif

// ADC Configuration
#define HW_ADC_VREF_VOLTAGE 3.3f        // Reference voltage (V)
#define HW_ADC_MAX_VALUE 4095           // 12-bit ADC (0-4095)
#define HW_ADC_CHANNEL_COUNT 4          // ADC channels available

// SPI Configuration
#define HW_SPI_MAX31855_FREQ 5000000    // 5MHz max for MAX31855
#define HW_SPI_MAX31855_DATA_BITS 32    // MAX31855 returns 32-bit data

// PWM Configuration
// Hardware PWM at 25Hz for standard heater control modes (PARALLEL, SEQUENTIAL, BREW_ONLY)
// Note: HEAT_SMART_STAGGER uses software GPIO control at 1Hz (1000ms period)
// for phase synchronization - see control_common.c
#define HW_PWM_FREQ_HZ 25               // 25Hz for SSR hardware PWM
#define HW_PWM_RESOLUTION_BITS 8        // 8-bit resolution (0-255)

// =============================================================================
// Initialization
// =============================================================================

/**
 * Initialize hardware peripherals
 * Must be called before using any hardware functions
 * 
 * @return true if initialization successful, false otherwise
 */
bool hw_init(void);

/**
 * Set simulation mode
 * 
 * @param enable true to enable simulation, false for real hardware
 */
void hw_set_simulation_mode(bool enable);

/**
 * Check if simulation mode is active
 * 
 * @return true if in simulation mode, false if using real hardware
 */
bool hw_is_simulation_mode(void);

// =============================================================================
// ADC (Analog-to-Digital Converter)
// =============================================================================

/**
 * Read ADC channel
 * 
 * @param channel ADC channel (0-3, maps to GPIO26-29)
 * @return ADC value (0-4095 for 12-bit), or 0 on error
 */
uint16_t hw_read_adc(uint8_t channel);

/**
 * Convert ADC value to voltage
 * 
 * @param adc_value Raw ADC reading (0-4095)
 * @return Voltage in volts (0.0 to 3.3V)
 */
float hw_adc_to_voltage(uint16_t adc_value);

/**
 * Read ADC channel and return voltage directly
 * 
 * @param channel ADC channel (0-3)
 * @return Voltage in volts, or 0.0 on error
 */
float hw_read_adc_voltage(uint8_t channel);

// =============================================================================
// SPI (Serial Peripheral Interface)
// =============================================================================

/**
 * Initialize SPI for MAX31855 thermocouple amplifier
 * 
 * @return true if successful, false otherwise
 */
bool hw_spi_init_max31855(void);

/**
 * Read MAX31855 thermocouple amplifier
 * 
 * @param data Pointer to store 32-bit data from MAX31855
 * @return true if read successful, false on error
 */
bool hw_spi_read_max31855(uint32_t* data);

/**
 * Convert MAX31855 data to temperature
 * 
 * @param data 32-bit data from MAX31855
 * @param temp_c Pointer to store temperature in Celsius
 * @return true if conversion successful, false on fault
 */
bool hw_max31855_to_temp(uint32_t data, float* temp_c);

/**
 * Check MAX31855 for fault conditions
 * 
 * @param data 32-bit data from MAX31855
 * @return true if fault detected, false if OK
 */
bool hw_max31855_is_fault(uint32_t data);

/**
 * Get MAX31855 fault code
 * 
 * @param data 32-bit data from MAX31855
 * @return Fault code: 0=OK, 1=OC (open circuit), 2=SCG (short to GND), 3=SCV (short to VCC)
 */
uint8_t hw_max31855_get_fault(uint32_t data);

// =============================================================================
// PWM (Pulse Width Modulation)
// =============================================================================

/**
 * PWM SSR configuration structure
 * Tracks both slice and channel to properly handle RP2350 PWM architecture
 * where two adjacent GPIOs share a slice but use different channels.
 */
typedef struct {
    uint8_t slice;      // PWM slice number (0-7)
    uint8_t channel;    // PWM channel (0=A, 1=B)
    uint8_t gpio_pin;   // GPIO pin for this PWM output
    bool initialized;   // True if this config has been initialized
} pwm_ssr_config_t;

/**
 * Initialize PWM for SSR (Solid State Relay) control
 * 
 * @param gpio_pin GPIO pin for PWM output
 * @param slice_num Pointer to store PWM slice number (for backward compatibility)
 * @return true if successful, false otherwise
 * 
 * @note Use hw_pwm_init_ssr_ex for proper channel tracking on new code
 */
bool hw_pwm_init_ssr(uint8_t gpio_pin, uint8_t* slice_num);

/**
 * Initialize PWM for SSR with full configuration tracking
 * Preferred over hw_pwm_init_ssr for proper channel handling.
 * 
 * @param gpio_pin GPIO pin for PWM output
 * @param config Pointer to config structure to populate
 * @return true if successful, false otherwise
 */
bool hw_pwm_init_ssr_ex(uint8_t gpio_pin, pwm_ssr_config_t* config);

/**
 * Set PWM duty cycle for SSR (legacy interface)
 * 
 * @param slice_num PWM slice number (from hw_pwm_init_ssr)
 * @param duty_percent Duty cycle (0.0 to 100.0)
 * 
 * @note This function uses internal tracking to determine the correct channel.
 *       Use hw_set_pwm_duty_ex with a config for guaranteed correct behavior.
 */
void hw_set_pwm_duty(uint8_t slice_num, float duty_percent);

/**
 * Set PWM duty cycle for SSR using full configuration
 * Preferred over hw_set_pwm_duty for proper channel handling.
 * 
 * @param config PWM configuration from hw_pwm_init_ssr_ex
 * @param duty_percent Duty cycle (0.0 to 100.0)
 */
void hw_set_pwm_duty_ex(const pwm_ssr_config_t* config, float duty_percent);

/**
 * Get current PWM duty cycle
 * 
 * @param slice_num PWM slice number
 * @return Duty cycle (0.0 to 100.0)
 */
float hw_get_pwm_duty(uint8_t slice_num);

/**
 * Enable/disable PWM output
 * 
 * @param slice_num PWM slice number
 * @param enable true to enable, false to disable
 */
void hw_pwm_set_enabled(uint8_t slice_num, bool enable);

// =============================================================================
// GPIO (General Purpose Input/Output)
// =============================================================================

/**
 * Configure GPIO pin as output
 * 
 * @param pin GPIO pin number
 * @param initial_state Initial output state (true=high, false=low)
 * @return true if successful, false otherwise
 */
bool hw_gpio_init_output(uint8_t pin, bool initial_state);

/**
 * Configure GPIO pin as input
 * 
 * @param pin GPIO pin number
 * @param pull_up Enable pull-up resistor
 * @param pull_down Enable pull-down resistor
 * @return true if successful, false otherwise
 */
bool hw_gpio_init_input(uint8_t pin, bool pull_up, bool pull_down);

/**
 * Set GPIO output state
 * 
 * @param pin GPIO pin number
 * @param state true=high, false=low
 */
void hw_set_gpio(uint8_t pin, bool state);

/**
 * Read GPIO input state
 * 
 * @param pin GPIO pin number
 * @return true if high, false if low
 */
bool hw_read_gpio(uint8_t pin);

/**
 * Toggle GPIO output
 * 
 * @param pin GPIO pin number
 */
void hw_toggle_gpio(uint8_t pin);

// =============================================================================
// Simulation Mode Helpers
// =============================================================================

/**
 * Set simulated ADC value (for testing)
 * 
 * @param channel ADC channel
 * @param value Simulated ADC value (0-4095)
 */
void hw_sim_set_adc(uint8_t channel, uint16_t value);

/**
 * Set simulated MAX31855 data (for testing)
 * 
 * @param data Simulated 32-bit data
 */
void hw_sim_set_max31855(uint32_t data);

/**
 * Set simulated GPIO state (for testing)
 * 
 * @param pin GPIO pin number
 * @param state Simulated state
 */
void hw_sim_set_gpio(uint8_t pin, bool state);

#endif // HARDWARE_H

