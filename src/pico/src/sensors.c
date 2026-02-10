/**
 * Pico Firmware - Sensor Reading
 * 
 * Handles reading of all sensors (temperature, pressure, water level).
 * Uses hardware abstraction layer for real hardware or simulation.
 */

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "sensors.h"
#include "config.h"
#include "hardware.h"
#include "sensor_utils.h"
#include "pcb_config.h"
#include "machine_config.h"
#include <stdlib.h>
#include <math.h>

// Machine-type awareness: sensors are only read if they exist for this machine type
// This prevents false fault detection and unnecessary processing

// =============================================================================
// Filter Configuration
// =============================================================================

// Median filter sizes (first stage - rejects noise spikes)
#define MEDIAN_FILTER_SIZE_BREW_NTC    5   // Median filter samples for brew NTC (odd number)
#define MEDIAN_FILTER_SIZE_STEAM_NTC   5   // Median filter samples for steam NTC (odd number)
#define MEDIAN_FILTER_SIZE_PRESSURE    3   // Median filter samples for pressure (odd number)

// Moving average filter sizes (second stage - smooths median output)
#define FILTER_SIZE_BREW_NTC    8   // Moving average samples for brew NTC
#define FILTER_SIZE_STEAM_NTC   8   // Moving average samples for steam NTC
#define FILTER_SIZE_PRESSURE    4   // Moving average samples for pressure

// =============================================================================
// Private State
// =============================================================================

static sensor_data_t g_sensor_data = {0};
static bool g_use_hardware = false;  // Use hardware abstraction (sim or real)

// Median filter buffers (first stage - rejects noise spikes)
static float g_median_buf_brew[MEDIAN_FILTER_SIZE_BREW_NTC];
static float g_median_buf_steam[MEDIAN_FILTER_SIZE_STEAM_NTC];
static float g_median_buf_pressure[MEDIAN_FILTER_SIZE_PRESSURE];

// Moving average filter buffers (second stage - smooths median output)
static float g_filter_buf_brew[FILTER_SIZE_BREW_NTC];
static float g_filter_buf_steam[FILTER_SIZE_STEAM_NTC];
static float g_filter_buf_pressure[FILTER_SIZE_PRESSURE];

// Median filter structures
static median_filter_t g_median_filter_brew;
static median_filter_t g_median_filter_steam;
static median_filter_t g_median_filter_pressure;

// Moving average filter structures
static moving_avg_filter_t g_filter_brew;
static moving_avg_filter_t g_filter_steam;
static moving_avg_filter_t g_filter_pressure;

// Simulation state (fallback if hardware abstraction is in simulation mode)
static float g_sim_brew_temp = 25.0f;
static float g_sim_steam_temp = 25.0f;
static bool g_sim_heating = false;

// Sensor fault tracking
static bool g_brew_ntc_fault = false;
static bool g_steam_ntc_fault = false;
static bool g_pressure_sensor_fault = false;

// Sensor error tracking (consecutive failures)
static uint16_t g_brew_ntc_error_count = 0;
static uint16_t g_steam_ntc_error_count = 0;
static uint16_t g_pressure_error_count = 0;

#define SENSOR_ERROR_THRESHOLD 10  // Report error after 10 consecutive failures

// Rate limit for sensor diagnostic logs (avoid log spam)
#define SENSOR_LOG_INTERVAL_MS  5000   // Log at most every 5 seconds
static uint32_t g_last_brew_ntc_log_ms = 0;
static uint32_t g_last_steam_ntc_log_ms = 0;
static uint32_t g_last_sensor_status_log_ms = 0;

// Last raw ADC values (for diagnostic log; 0xFFFF = not read)
#define ADC_NOT_READ  0xFFFFu
static uint16_t g_last_brew_adc = ADC_NOT_READ;
static uint16_t g_last_steam_adc = ADC_NOT_READ;

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * Read brew NTC thermistor
 * Returns NAN if sensor doesn't exist for this machine type
 */
static float read_brew_ntc(void) {
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Machine-type check: HX machines don't have brew NTC
    if (!machine_has_brew_ntc()) {
        if (now_ms - g_last_brew_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_brew_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Brew NTC not read (machine type has no brew NTC)\n");
        }
        return NAN;  // Not present on this machine type
    }

    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || pcb->pins.adc_brew_ntc < 0) {
        if (now_ms - g_last_brew_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_brew_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Brew NTC not read (pin not configured)\n");
        }
        return NAN;  // Not configured
    }

    // Get ADC channel (GPIO26=0, GPIO27=1, GPIO28=2, GPIO29=3)
    uint8_t adc_channel = pcb->pins.adc_brew_ntc - 26;
    if (adc_channel > 3) {
        if (now_ms - g_last_brew_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_brew_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Brew NTC not read (invalid ADC channel %u)\n", (unsigned)adc_channel);
        }
        return NAN;  // Invalid channel
    }

    // Read ADC
    uint16_t adc_value = hw_read_adc(adc_channel);
    g_last_brew_adc = adc_value;

    // Convert to temperature (brew: 3.3k series; 50k or 3.3k NTC per PCB)
    float temp_c = ntc_adc_to_temp(
        adc_value,
        HW_ADC_VREF_VOLTAGE,
        NTC_SERIES_BREW_OHMS,
        NTC_R25_OHMS,
        NTC_B_VALUE
    );

    // Validate
    if (!sensor_validate_temp(temp_c, -10.0f, 200.0f)) {
        g_brew_ntc_fault = true;
        g_brew_ntc_error_count++;
        if (g_brew_ntc_error_count >= SENSOR_ERROR_THRESHOLD &&
            g_brew_ntc_error_count == SENSOR_ERROR_THRESHOLD) {
            LOG_PRINT("Sensors: ERROR - Brew NTC invalid reading (%.1fC) - %d consecutive failures\n",
                       temp_c, g_brew_ntc_error_count);
        } else if (now_ms - g_last_brew_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_brew_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Brew NTC invalid (ADC=%u -> %.1fC, out of range -10..200C)\n",
                      (unsigned)adc_value, (double)temp_c);
        }
        return NAN;
    }

    // Valid reading - reset error count
    if (g_brew_ntc_error_count > 0) {
        LOG_PRINT("Sensors: Brew NTC recovered after %d failures\n", g_brew_ntc_error_count);
    }
    g_brew_ntc_fault = false;
    g_brew_ntc_error_count = 0;
    return temp_c;
}

/**
 * Read steam NTC thermistor
 * Returns NAN if sensor doesn't exist for this machine type
 */
static float read_steam_ntc(void) {
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Machine-type check: Single boiler machines don't have steam NTC
    if (!machine_has_steam_ntc()) {
        if (now_ms - g_last_steam_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_steam_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Steam NTC not read (machine type has no steam NTC)\n");
        }
        return NAN;  // Not present on this machine type
    }

    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || pcb->pins.adc_steam_ntc < 0) {
        if (now_ms - g_last_steam_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_steam_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Steam NTC not read (pin not configured)\n");
        }
        return NAN;  // Not configured
    }

    // Get ADC channel
    uint8_t adc_channel = pcb->pins.adc_steam_ntc - 26;
    if (adc_channel > 3) {
        if (now_ms - g_last_steam_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_steam_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Steam NTC not read (invalid ADC channel %u)\n", (unsigned)adc_channel);
        }
        return NAN;  // Invalid channel
    }

    // Read ADC
    uint16_t adc_value = hw_read_adc(adc_channel);
    g_last_steam_adc = adc_value;

    // Convert to temperature (steam: 1.2k series on ECM, else 3.3k; 50k or 3.3k NTC per PCB)
    float temp_c = ntc_adc_to_temp(
        adc_value,
        HW_ADC_VREF_VOLTAGE,
        NTC_SERIES_STEAM_OHMS,
        NTC_R25_OHMS,
        NTC_B_VALUE
    );

    // Validate
    if (!sensor_validate_temp(temp_c, -10.0f, 200.0f)) {
        g_steam_ntc_fault = true;
        g_steam_ntc_error_count++;
        if (g_steam_ntc_error_count >= SENSOR_ERROR_THRESHOLD &&
            g_steam_ntc_error_count == SENSOR_ERROR_THRESHOLD) {
            LOG_PRINT("Sensors: ERROR - Steam NTC invalid reading (%.1fC) - %d consecutive failures\n",
                       temp_c, g_steam_ntc_error_count);
        } else if (now_ms - g_last_steam_ntc_log_ms >= SENSOR_LOG_INTERVAL_MS) {
            g_last_steam_ntc_log_ms = now_ms;
            LOG_PRINT("Sensors: Steam NTC invalid (ADC=%u -> %.1fC, out of range -10..200C)\n",
                      (unsigned)adc_value, (double)temp_c);
        }
        return NAN;
    }

    // Valid reading - reset error count
    if (g_steam_ntc_error_count > 0) {
        LOG_PRINT("Sensors: Steam NTC recovered after %d failures\n", g_steam_ntc_error_count);
    }
    g_steam_ntc_fault = false;
    g_steam_ntc_error_count = 0;
    return temp_c;
}

/**
 * Read group temperature
 * Returns NAN as group head thermocouple support was removed (v2.24.3)
 */
static float read_group_thermocouple(void) {
    // Group head thermocouple (MAX31855) support removed in v2.24.3
    // Boiler NTC sensors provide sufficient temperature control
    return NAN;
}

/**
 * Read pressure sensor
 */
static float read_pressure(void) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || pcb->pins.adc_pressure < 0) {
        return 0.0f;  // Not configured, return 0 bar
    }
    
    // Get ADC channel
    uint8_t adc_channel = pcb->pins.adc_pressure - 26;
    if (adc_channel > 3) {
        return 0.0f;  // Invalid channel
    }
    
    // Read ADC voltage
    float voltage = hw_read_adc_voltage(adc_channel);
    
    // Validate voltage range (sanity check)
    // Expected range: 0.3V to 2.7V (after voltage divider: 0.5V to 4.5V transducer)
    // Add some margin: 0.2V to 3.0V
    if (voltage < 0.2f || voltage > 3.0f) {
        g_pressure_sensor_fault = true;
        g_pressure_error_count++;
        if (g_pressure_error_count >= SENSOR_ERROR_THRESHOLD && 
            g_pressure_error_count == SENSOR_ERROR_THRESHOLD) {
            DEBUG_PRINT("SENSOR ERROR: Pressure sensor voltage out of range (%.2fV) - %d consecutive failures\n", 
                       voltage, g_pressure_error_count);
        }
        return 0.0f;  // Return 0 bar on error
    }
    
    // YD4060 pressure transducer:
    // - 0.5V = 0 bar
    // - 4.5V = 16 bar
    // - Voltage divider: R3=10kΩ (pull-down), R4=5.6kΩ (series)
    // - Ratio = R3/(R3+R4) = 10k/(10k+5.6k) = 10k/15.6k = 0.641
    // So ADC sees: V_adc = V_transducer * 0.641
    // V_transducer = V_adc / 0.641
    
    // Pressure divider ratio (R3=10kΩ, R4=5.6kΩ)
    #define PRESSURE_DIVIDER_RATIO 0.641f  // 10kΩ / (10kΩ + 5.6kΩ)
    
    float v_transducer_nominal = voltage / PRESSURE_DIVIDER_RATIO;
    
    // Ratiometric compensation: normalize to what sensor would read at 5.0V
    // If 5V rail sags (e.g., to 4.5V), sensor output drops proportionally
    // We must DIVIDE by the ratio (or multiply by inverse) to correct it
    float v_transducer = v_transducer_nominal;
    
    const pcb_config_t* pcb_5v = pcb_config_get();
    if (pcb_5v && pcb_5v->pins.adc_5v_monitor >= 0) {
        // Read 5V rail monitor (GPIO29, ADC3)
        uint8_t adc_5v_channel = pcb_5v->pins.adc_5v_monitor - 26;
        if (adc_5v_channel <= 3) {
            float v_adc_5v = hw_read_adc_voltage(adc_5v_channel);
            
            // Calculate actual 5V rail voltage (divider: R91=10kΩ, R92=5.6kΩ)
            // V_5V_actual = V_adc_5v × (R91+R92)/R92 = V_adc_5v × 15.6k/5.6k = V_adc_5v × 2.786
            float v_5v_actual = v_adc_5v * 2.786f;
            
            // Validate 5V rail reading (sanity check: 4.0V to 5.5V)
            if (v_5v_actual >= 4.0f && v_5v_actual <= 5.5f) {
                // Apply ratiometric compensation: multiply by inverse ratio
                // Formula: V_sensor_actual = V_sensor_nominal × (5.0V / V_5V_actual)
                v_transducer = v_transducer_nominal * (5.0f / v_5v_actual);
            }
            // If 5V reading is invalid, use nominal value (fallback)
        }
    }
    
    // Validate transducer voltage range
    if (v_transducer < 0.3f || v_transducer > 4.7f) {
        g_pressure_sensor_fault = true;
        g_pressure_error_count++;
        if (g_pressure_error_count >= SENSOR_ERROR_THRESHOLD && 
            g_pressure_error_count == SENSOR_ERROR_THRESHOLD) {
            DEBUG_PRINT("SENSOR ERROR: Pressure transducer voltage out of range (%.2fV) - %d consecutive failures\n", 
                       v_transducer, g_pressure_error_count);
        }
        return 0.0f;
    }
    
    // Convert to pressure (0.5V to 4.5V = 0 to 16 bar)
    float pressure_bar = (v_transducer - 0.5f) * 16.0f / 4.0f;
    
    // Clamp to valid range
    if (pressure_bar < 0.0f) pressure_bar = 0.0f;
    if (pressure_bar > 16.0f) pressure_bar = 16.0f;
    
    // Valid reading - reset error count
    if (g_pressure_error_count > 0) {
        DEBUG_PRINT("SENSOR: Pressure sensor recovered after %d failures\n", g_pressure_error_count);
    }
    g_pressure_sensor_fault = false;
    g_pressure_error_count = 0;
    
    return pressure_bar;
}

/**
 * Read water level (digital inputs)
 * Switches are active HIGH = OK (full), LOW = empty/low
 */
static uint8_t read_water_level(void) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb) {
        return 100;  // Assume full if not configured
    }

    // Determine water mode: plumbed vs water tank
    bool is_plumbed = false;
    if (pcb->pins.input_water_mode >= 0) {
        is_plumbed = hw_read_gpio(pcb->pins.input_water_mode);  // HIGH = plumbed
    }

    // In plumbed mode, water line is always available - only check steam level
    if (is_plumbed) {
        if (pcb->pins.input_steam_level >= 0) {
            // Steam level probe: LOW = water present (AC attenuated), HIGH = empty (AC passes)
            bool steam_ok = !hw_read_gpio(pcb->pins.input_steam_level);
            return steam_ok ? 100 : 50;
        }
        return 100;  // Plumbed, all OK
    }

    // Water tank mode: check tank level sensor (magnetic float)
    bool tank_level_ok = true;
    bool steam_level_ok = true;

    if (pcb->pins.input_tank_level >= 0) {
        // Magnetic float switch: HIGH = water ok, LOW = empty
        tank_level_ok = hw_read_gpio(pcb->pins.input_tank_level);
    }

    if (pcb->pins.input_steam_level >= 0) {
        // Steam level probe: LOW = water present (AC attenuated), HIGH = empty (AC passes)
        steam_level_ok = !hw_read_gpio(pcb->pins.input_steam_level);
    }

    if (!tank_level_ok) {
        return 0;  // Tank empty - critical
    }

    if (!steam_level_ok) {
        return 50;  // Steam boiler low
    }

    return 100;  // All OK
}

/**
 * Log water level probe raw state (for diagnostics).
 * Call with pcb and current level; logs GPIO pin, raw value, and interpretation.
 */
static void log_water_level_probes(const pcb_config_t* pcb, uint8_t level) {
    if (!pcb) return;

    int8_t wm_pin = pcb->pins.input_water_mode;
    int8_t t_pin  = pcb->pins.input_tank_level;
    int8_t s_pin  = pcb->pins.input_steam_level;

    // Water mode: HIGH=plumbed, LOW=tank
    const char* wm_str = (wm_pin < 0) ? "n/c" :
                          (hw_read_gpio((uint8_t)wm_pin) ? "PLUMBED" : "TANK");
    // Tank level (magnetic float): HIGH=ok, LOW=empty
    const char* t_str = (t_pin < 0) ? "n/c" :
                         (hw_read_gpio((uint8_t)t_pin) ? "OK" : "EMPTY");
    // Steam level (AC probe): LOW=water present (OK), HIGH=dry (LOW)
    const char* s_str = (s_pin < 0) ? "n/c" :
                         (hw_read_gpio((uint8_t)s_pin) ? "EMPTY" : "OK");

    LOG_PRINT("Sensors: Water mode(GP%d)=%s tank(GP%d)=%s steam(GP%d)=%s => %u%%\n",
              (int)wm_pin, wm_str, (int)t_pin, t_str, (int)s_pin, s_str, (unsigned)level);
}

// =============================================================================
// Initialization
// =============================================================================

void sensors_init(void) {
    // Initialize median filters (first stage - rejects noise spikes)
    filter_median_init(&g_median_filter_brew, g_median_buf_brew, MEDIAN_FILTER_SIZE_BREW_NTC);
    filter_median_init(&g_median_filter_steam, g_median_buf_steam, MEDIAN_FILTER_SIZE_STEAM_NTC);
    filter_median_init(&g_median_filter_pressure, g_median_buf_pressure, MEDIAN_FILTER_SIZE_PRESSURE);
    
    // Initialize moving average filters (second stage - smooths median output)
    filter_moving_avg_init(&g_filter_brew, g_filter_buf_brew, FILTER_SIZE_BREW_NTC);
    filter_moving_avg_init(&g_filter_steam, g_filter_buf_steam, FILTER_SIZE_STEAM_NTC);
    filter_moving_avg_init(&g_filter_pressure, g_filter_buf_pressure, FILTER_SIZE_PRESSURE);
    
    // Check if hardware abstraction is available
    g_use_hardware = true;  // Always use hardware abstraction (sim or real)
    
    // Initialize digital inputs for water level
    // Note: gpio_init_inputs() in gpio_init.c handles the primary init.
    // This is a safety net for pins that might not be covered there.
    const pcb_config_t* pcb = pcb_config_get();
    if (pcb) {
        if (pcb->pins.input_reservoir >= 0) {
            hw_gpio_init_input(pcb->pins.input_reservoir, true, false);  // Pull-up (switch)
        }
        if (pcb->pins.input_tank_level >= 0) {
            hw_gpio_init_input(pcb->pins.input_tank_level, true, false);  // Pull-up (magnetic float)
        }
        // Steam level (GPIO4): NO pull-up - TLV3201 comparator drives this pin
        // Adding a pull-up would fight the comparator output
    }
    
    // Initialize sensor data with default values
    g_sensor_data.brew_temp = 250;      // 25.0C
    g_sensor_data.steam_temp = 250;    // 25.0C
    g_sensor_data.group_temp = 250;    // 25.0C
    g_sensor_data.pressure = 0;        // 0.00 bar
    g_sensor_data.water_level = 80;    // 80%
    
    LOG_PRINT("Sensors: Initialized (mode: %s, brew_ntc: %s, steam_ntc: %s, NTC: %.0fR@25C series=%.0fR)\n",
              hw_is_simulation_mode() ? "SIMULATION" : "REAL",
              machine_has_brew_ntc() ? "yes" : "no",
              machine_has_steam_ntc() ? "yes" : "no",
              (double)NTC_R25_OHMS, (double)NTC_SERIES_R_OHMS);
}

// =============================================================================
// Sensor Reading
// =============================================================================

void sensors_read(void) {
    if (g_use_hardware) {
        // Use hardware abstraction layer (works in both sim and real mode)
        
        // Read and filter brew NTC (two-stage: median + moving average)
        float brew_temp_raw = read_brew_ntc();
        if (!isnan(brew_temp_raw)) {
            // First stage: median filter rejects noise spikes
            float brew_temp_median = filter_median_update(&g_median_filter_brew, brew_temp_raw);
            // Second stage: moving average smooths the median output
            float brew_temp_filtered = filter_moving_avg_update(&g_filter_brew, brew_temp_median);
            g_sensor_data.brew_temp = (int16_t)(brew_temp_filtered * 10.0f);
        } else {
            // Sensor fault - keep last valid value (filter maintains it)
            // Safety system will detect NAN and handle appropriately
        }
        
        // Read and filter steam NTC (two-stage: median + moving average)
        float steam_temp_raw = read_steam_ntc();
        if (!isnan(steam_temp_raw)) {
            // First stage: median filter rejects noise spikes
            float steam_temp_median = filter_median_update(&g_median_filter_steam, steam_temp_raw);
            // Second stage: moving average smooths the median output
            float steam_temp_filtered = filter_moving_avg_update(&g_filter_steam, steam_temp_median);
            g_sensor_data.steam_temp = (int16_t)(steam_temp_filtered * 10.0f);
        } else {
            // Sensor fault - keep last valid value
        }
        
        // Note: Group head thermocouple support removed (v2.24.3)
        // g_sensor_data.group_temp remains at default/last value (NAN data)
        
        // Read and filter pressure (two-stage: median + moving average)
        float pressure_raw = read_pressure();
        if (!g_pressure_sensor_fault) {
            // Only update filter if sensor is not in fault state
            // First stage: median filter rejects noise spikes
            float pressure_median = filter_median_update(&g_median_filter_pressure, pressure_raw);
            // Second stage: moving average smooths the median output
            float pressure_filtered = filter_moving_avg_update(&g_filter_pressure, pressure_median);
            g_sensor_data.pressure = (uint16_t)(pressure_filtered * 100.0f);  // Convert to 0.01 bar units
        } else {
            // Sensor fault - keep last valid value (filter maintains it)
        }
        
        // Read water level
        g_sensor_data.water_level = read_water_level();

        // Periodic sensor status log (rate-limited) for diagnostics
        {
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            if (now_ms - g_last_sensor_status_log_ms >= SENSOR_LOG_INTERVAL_MS) {
                g_last_sensor_status_log_ms = now_ms;
                float brew_c = g_sensor_data.brew_temp / 10.0f;
                float steam_c = g_sensor_data.steam_temp / 10.0f;
                LOG_PRINT("Sensors: brew=%.1fC (GP26/ADC0=%u) steam=%.1fC (GP27/ADC1=%u)\n",
                          (double)brew_c, (unsigned)g_last_brew_adc,
                          (double)steam_c, (unsigned)g_last_steam_adc);
                log_water_level_probes(pcb_config_get(), g_sensor_data.water_level);
            }
        }

        // Hardware power metering removed (v2.32). Power monitoring via MQTT smart plugs on ESP32.

    } else {
        // Fallback: Old simulation mode (should not be used if hardware abstraction is available)
        // This is kept for backward compatibility
        
        // Target temperatures based on whether heating is "on"
        float brew_target = g_sim_heating ? 93.0f : 25.0f;
        float steam_target = g_sim_heating ? 140.0f : 25.0f;
        
        // Slowly approach target (thermal mass simulation)
        float rate = 0.1f;  // Degrees per 50ms read cycle
        
        if (g_sim_brew_temp < brew_target) {
            g_sim_brew_temp += rate;
            if (g_sim_brew_temp > brew_target) g_sim_brew_temp = brew_target;
        } else if (g_sim_brew_temp > brew_target) {
            g_sim_brew_temp -= rate * 0.3f;  // Cool slower
            if (g_sim_brew_temp < brew_target) g_sim_brew_temp = brew_target;
        }
        
        if (g_sim_steam_temp < steam_target) {
            g_sim_steam_temp += rate * 0.8f;
            if (g_sim_steam_temp > steam_target) g_sim_steam_temp = steam_target;
        } else if (g_sim_steam_temp > steam_target) {
            g_sim_steam_temp -= rate * 0.2f;
            if (g_sim_steam_temp < steam_target) g_sim_steam_temp = steam_target;
        }
        
        // Add small noise for realism
        float noise = ((float)(rand() % 10) - 5.0f) / 50.0f;
        
        g_sensor_data.brew_temp = (int16_t)((g_sim_brew_temp + noise) * 10);
        g_sensor_data.steam_temp = (int16_t)((g_sim_steam_temp + noise) * 10);
        g_sensor_data.group_temp = (int16_t)((g_sim_brew_temp - 5.0f + noise) * 10);
        g_sensor_data.pressure = 100 + (rand() % 20);  // ~1.0 bar
    }
}

void sensors_get_data(sensor_data_t* data) {
    if (data) {
        *data = g_sensor_data;
    }
}

// =============================================================================
// Individual Sensor Access
// =============================================================================

int16_t sensors_get_brew_temp(void) {
    return g_sensor_data.brew_temp;
}

int16_t sensors_get_steam_temp(void) {
    return g_sensor_data.steam_temp;
}

int16_t sensors_get_group_temp(void) {
    return g_sensor_data.group_temp;
}

uint16_t sensors_get_pressure(void) {
    return g_sensor_data.pressure;
}

uint8_t sensors_get_water_level(void) {
    return g_sensor_data.water_level;
}

// =============================================================================
// Simulation Control (for development)
// =============================================================================

void sensors_set_simulation(bool enable) {
    // This now controls the hardware abstraction layer's simulation mode
    hw_set_simulation_mode(enable);
    DEBUG_PRINT("Sensor simulation: %s\n", enable ? "enabled" : "disabled");
}

void sensors_sim_set_heating(bool heating) {
    g_sim_heating = heating;
    
    // If in simulation mode, set simulated ADC values to simulate heating
    if (hw_is_simulation_mode()) {
        // Calculate ADC values for target temperatures
        // This is approximate - in real use, the hardware abstraction layer
        // would handle simulation values
        
        // For brew: target 93°C when heating
        // For steam: target 140°C when heating
        // These would be set via hw_sim_set_adc() if needed
    }
}
