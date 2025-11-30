/**
 * ECM Pico Firmware - Control System Common Implementation
 * 
 * Shared control code used by all machine types:
 * - PID computation with derivative filtering
 * - Hardware output control (SSRs, relays)
 * - Heating strategy implementation (dual boiler)
 * - Public API wrappers
 */

#include "pico/stdlib.h"
#include "control.h"
#include "control_impl.h"
#include "config.h"
#include "sensors.h"
#include "protocol.h"
#include "machine_electrical.h"
#include "environmental_config.h"
#include "hardware.h"
#include "pcb_config.h"
#include "machine_config.h"
#include "safety.h"
#include "state.h"
#include "pzem.h"
#include <math.h>
#include <string.h>

// =============================================================================
// Global State (shared with machine-specific implementations)
// =============================================================================

pid_state_t g_brew_pid;
pid_state_t g_steam_pid;
heating_strategy_t g_heating_strategy = HEAT_SEQUENTIAL;

// =============================================================================
// Private State
// =============================================================================

static control_outputs_t g_outputs = {0};
static bool g_outputs_initialized = false;

// Hardware PWM slices
static uint8_t g_pwm_slice_brew = 0xFF;
static uint8_t g_pwm_slice_steam = 0xFF;

// Heating strategy configuration
static float g_sequential_threshold_pct = 80.0f;
static float g_max_combined_duty = 150.0f;
static uint8_t g_stagger_priority = 0;

// =============================================================================
// PID Initialization
// =============================================================================

void pid_init(pid_state_t* pid, float setpoint) {
    pid->kp = PID_DEFAULT_KP;
    pid->ki = PID_DEFAULT_KI;
    pid->kd = PID_DEFAULT_KD;
    pid->setpoint = setpoint;
    pid->setpoint_target = setpoint;
    pid->integral = 0;
    pid->last_error = 0;
    pid->last_derivative = 0;
    pid->output = 0;
    pid->setpoint_ramping = false;
    pid->ramp_rate = 1.0f;
}

// =============================================================================
// PID Computation with Derivative Filtering
// =============================================================================

float pid_compute(pid_state_t* pid, float process_value, float dt) {
    // Update setpoint with ramping if enabled
    if (pid->setpoint_ramping) {
        float setpoint_diff = pid->setpoint_target - pid->setpoint;
        float max_change = pid->ramp_rate * dt;
        
        if (fabsf(setpoint_diff) <= max_change) {
            pid->setpoint = pid->setpoint_target;
            pid->setpoint_ramping = false;
        } else {
            if (setpoint_diff > 0) {
                pid->setpoint += max_change;
            } else {
                pid->setpoint -= max_change;
            }
        }
    }
    
    float error = pid->setpoint - process_value;
    
    // Proportional
    float p_term = pid->kp * error;
    
    // Integral with anti-windup
    // Guard against division by zero when ki is 0 or very small (P-only control)
    float i_term = 0.0f;
    if (pid->ki > 0.001f) {
        pid->integral += error * dt;
        float max_integral = PID_OUTPUT_MAX / pid->ki;
        if (pid->integral > max_integral) pid->integral = max_integral;
        if (pid->integral < -max_integral) pid->integral = -max_integral;
        i_term = pid->ki * pid->integral;
    } else {
        // Ki disabled or negligible - reset integral to prevent windup when re-enabled
        pid->integral = 0.0f;
    }
    
    // Derivative with filtering
    float derivative = (error - pid->last_error) / dt;
    float alpha = 0.1f;
    pid->last_derivative = alpha * derivative + (1.0f - alpha) * pid->last_derivative;
    float d_term = pid->kd * pid->last_derivative;
    
    pid->last_error = error;
    
    // Sum and clamp
    float output = p_term + i_term + d_term;
    if (output > PID_OUTPUT_MAX) output = PID_OUTPUT_MAX;
    if (output < PID_OUTPUT_MIN) output = PID_OUTPUT_MIN;
    
    pid->output = output;
    return output;
}

// =============================================================================
// Heating Strategy Implementation (Dual Boiler Only)
// =============================================================================

void apply_heating_strategy(
    float brew_demand, float steam_demand,
    float brew_temp, float steam_temp,
    float* brew_duty, float* steam_duty
) {
    float brew_setpoint = g_brew_pid.setpoint;
    float steam_setpoint = g_steam_pid.setpoint;
    
    electrical_state_t elec_state;
    electrical_state_get(&elec_state);
    
    switch (g_heating_strategy) {
        case HEAT_BREW_ONLY:
            *brew_duty = brew_demand;
            *steam_duty = 0.0f;
            break;
            
        case HEAT_SEQUENTIAL: {
            *brew_duty = brew_demand;
            float brew_pct = (brew_temp / brew_setpoint) * 100.0f;
            *steam_duty = (brew_pct >= g_sequential_threshold_pct) ? steam_demand : 0.0f;
            break;
        }
            
        case HEAT_PARALLEL: {
            float brew_current = elec_state.brew_heater_current * (brew_demand / 100.0f);
            float steam_current = elec_state.steam_heater_current * (steam_demand / 100.0f);
            float total_current = brew_current + steam_current;
            
            if (total_current > elec_state.max_combined_current) {
                float scale_factor = elec_state.max_combined_current / total_current;
                brew_demand *= scale_factor;
                steam_demand *= scale_factor;
            }
            *brew_duty = brew_demand;
            *steam_duty = steam_demand;
            break;
        }
            
        case HEAT_STEAM_PRIORITY: {
            *steam_duty = steam_demand;
            float steam_pct = (steam_temp / steam_setpoint) * 100.0f;
            *brew_duty = (steam_pct >= g_sequential_threshold_pct) ? brew_demand : 0.0f;
            break;
        }
            
        case HEAT_SMART_STAGGER: {
            float brew_current = elec_state.brew_heater_current * (brew_demand / 100.0f);
            float steam_current = elec_state.steam_heater_current * (steam_demand / 100.0f);
            float total_current = brew_current + steam_current;
            
            if (total_current <= elec_state.max_combined_current) {
                *brew_duty = brew_demand;
                *steam_duty = steam_demand;
            } else {
                if (g_stagger_priority == 0) {
                    *brew_duty = fminf(brew_demand, 100.0f);
                    float remaining = elec_state.max_combined_current - 
                                     (elec_state.brew_heater_current * (*brew_duty / 100.0f));
                    *steam_duty = (remaining > 0) ? 
                        fminf(steam_demand, (remaining / elec_state.steam_heater_current) * 100.0f) : 0.0f;
                } else {
                    *steam_duty = fminf(steam_demand, 100.0f);
                    float remaining = elec_state.max_combined_current - 
                                     (elec_state.steam_heater_current * (*steam_duty / 100.0f));
                    *brew_duty = (remaining > 0) ? 
                        fminf(brew_demand, (remaining / elec_state.brew_heater_current) * 100.0f) : 0.0f;
                }
            }
            break;
        }
            
        default:
            *brew_duty = brew_demand;
            *steam_duty = 0.0f;
            break;
    }
    
    // Apply safety limit
    float max_duty = 95.0f;
    *brew_duty = fminf(*brew_duty, max_duty);
    *steam_duty = fminf(*steam_duty, max_duty);
}

// =============================================================================
// Hardware Output Control
// =============================================================================

bool init_hardware_outputs(void) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb) return false;
    
    // Initialize PWM for SSR control
    if (pcb->pins.ssr_brew >= 0) {
        if (hw_pwm_init_ssr(pcb->pins.ssr_brew, &g_pwm_slice_brew)) {
            DEBUG_PRINT("Brew SSR PWM initialized (slice %d)\n", g_pwm_slice_brew);
        }
    }
    
    if (pcb->pins.ssr_steam >= 0) {
        if (hw_pwm_init_ssr(pcb->pins.ssr_steam, &g_pwm_slice_steam)) {
            DEBUG_PRINT("Steam SSR PWM initialized (slice %d)\n", g_pwm_slice_steam);
        }
    }
    
    // Initialize relay outputs
    if (pcb->pins.relay_pump >= 0) {
        hw_gpio_init_output(pcb->pins.relay_pump, false);
    }
    if (pcb->pins.relay_brew_solenoid >= 0) {
        hw_gpio_init_output(pcb->pins.relay_brew_solenoid, false);
    }
    
    g_outputs_initialized = true;
    return true;
}

void apply_hardware_outputs(uint8_t brew_heater, uint8_t steam_heater, uint8_t pump) {
    const pcb_config_t* pcb = pcb_config_get();
    if (!pcb || !g_outputs_initialized) return;
    
    // Apply SSR outputs
    if (g_pwm_slice_brew != 0xFF && pcb->pins.ssr_brew >= 0) {
        hw_set_pwm_duty(g_pwm_slice_brew, (float)brew_heater);
    }
    
    if (g_pwm_slice_steam != 0xFF && pcb->pins.ssr_steam >= 0) {
        hw_set_pwm_duty(g_pwm_slice_steam, (float)steam_heater);
    }
    
    // Apply relay outputs
    if (pcb->pins.relay_pump >= 0) {
        hw_set_gpio(pcb->pins.relay_pump, pump > 0);
    }
}

uint16_t estimate_power_watts(uint8_t brew_duty, uint8_t steam_duty) {
    const machine_electrical_t* machine_elec = &MACHINE_ELECTRICAL_CONFIG;
    uint16_t brew_watts = (brew_duty * machine_elec->brew_heater_power) / 100;
    uint16_t steam_watts = (steam_duty * machine_elec->steam_heater_power) / 100;
    return brew_watts + steam_watts;
}

// =============================================================================
// Public API: Initialization
// =============================================================================

void control_init(void) {
    // Initialize PIDs with default values
    pid_init(&g_brew_pid, DEFAULT_BREW_TEMP / 10.0f);
    pid_init(&g_steam_pid, DEFAULT_STEAM_TEMP / 10.0f);
    
    // Initialize hardware
    init_hardware_outputs();
    
    // Machine-specific initialization
    control_init_machine();
    
    DEBUG_PRINT("Control initialized. Brew SP=%.1fC, Steam SP=%.1fC\n",
                g_brew_pid.setpoint, g_steam_pid.setpoint);
    
    // Initialize PZEM if available
    if (pzem_is_available()) {
        if (pzem_init(PZEM_DEFAULT_ADDRESS)) {
            DEBUG_PRINT("PZEM: Power meter initialized\n");
        }
    }
}

// =============================================================================
// Public API: Control Update
// =============================================================================

void control_update(void) {
    // Don't update if in safe state
    if (safety_is_safe_state()) {
        g_outputs.brew_heater = 0;
        g_outputs.steam_heater = 0;
        g_outputs.pump = 0;
        apply_hardware_outputs(0, 0, 0);
        return;
    }
    
    // Don't heat if machine is in IDLE mode
    machine_mode_t mode = state_get_mode();
    if (mode == MODE_IDLE) {
        g_outputs.brew_heater = 0;
        g_outputs.steam_heater = 0;
        apply_hardware_outputs(0, 0, g_outputs.pump);
        return;
    }
    
    // Read sensors
    sensor_data_t sensors;
    sensors_get_data(&sensors);
    
    float dt = CONTROL_LOOP_PERIOD_MS / 1000.0f;
    float brew_temp = sensors.brew_temp / 10.0f;
    float steam_temp = sensors.steam_temp / 10.0f;
    float group_temp = sensors.group_temp / 10.0f;
    
    // Machine-specific control logic
    float brew_duty = 0.0f;
    float steam_duty = 0.0f;
    
    control_update_machine(mode, brew_temp, steam_temp, group_temp, dt,
                          &brew_duty, &steam_duty);
    
    // Store and apply outputs
    g_outputs.brew_heater = (uint8_t)brew_duty;
    g_outputs.steam_heater = (uint8_t)steam_duty;
    apply_hardware_outputs(g_outputs.brew_heater, g_outputs.steam_heater, g_outputs.pump);
    
    // Update simulation
    sensors_sim_set_heating(g_outputs.brew_heater > 0 || g_outputs.steam_heater > 0);
    
    // Power measurement
    pzem_data_t pzem_data;
    if (pzem_is_available() && pzem_get_data(&pzem_data) && pzem_data.valid) {
        g_outputs.power_watts = pzem_data.power;
    } else {
        g_outputs.power_watts = estimate_power_watts(g_outputs.brew_heater, g_outputs.steam_heater);
    }
}

// =============================================================================
// Public API: Setpoint Control
// =============================================================================

void control_set_setpoint(uint8_t target, int16_t temp) {
    float temp_c = temp / 10.0f;
    pid_state_t* pid = (target == 0) ? &g_brew_pid : &g_steam_pid;
    
    pid->setpoint_target = temp_c;
    pid->setpoint_ramping = true;
    
    DEBUG_PRINT("%s setpoint: %.1fC\n", target == 0 ? "Brew" : "Steam", temp_c);
}

int16_t control_get_setpoint(uint8_t target) {
    pid_state_t* pid = (target == 0) ? &g_brew_pid : &g_steam_pid;
    return (int16_t)(pid->setpoint * 10);
}

// =============================================================================
// Public API: PID Tuning
// =============================================================================

void control_set_pid(uint8_t target, float kp, float ki, float kd) {
    if (target > 1) return;
    if (kp < 0.0f || ki < 0.0f || kd < 0.0f) return;
    if (kp > 100.0f || ki > 100.0f || kd > 100.0f) return;
    
    pid_state_t* pid = (target == 0) ? &g_brew_pid : &g_steam_pid;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0;
    pid->last_error = 0;
    pid->last_derivative = 0;
    
    DEBUG_PRINT("PID[%d] set: Kp=%.2f Ki=%.2f Kd=%.2f\n", target, kp, ki, kd);
}

void control_get_pid(uint8_t target, float* kp, float* ki, float* kd) {
    if (!kp || !ki || !kd) return;
    pid_state_t* pid = (target == 0) ? &g_brew_pid : &g_steam_pid;
    *kp = pid->kp;
    *ki = pid->ki;
    *kd = pid->kd;
}

// =============================================================================
// Public API: Output Control
// =============================================================================

void control_set_output(uint8_t output, uint8_t value, uint8_t mode) {
    if (value > 100) value = 100;
    if (mode > 1) return;
    
    switch (output) {
        case 0:
            g_outputs.brew_heater = value;
            break;
        case 1:
            g_outputs.steam_heater = value;
            break;
        case 2:
            g_outputs.pump = value;
            break;
        default:
            return;
    }
    
    if (mode == 1) {
        apply_hardware_outputs(g_outputs.brew_heater, g_outputs.steam_heater, g_outputs.pump);
    }
}

void control_get_outputs(control_outputs_t* outputs) {
    if (outputs) *outputs = g_outputs;
}

void control_set_pump(uint8_t value) {
    if (value > 100) value = 100;
    g_outputs.pump = value;
    apply_hardware_outputs(g_outputs.brew_heater, g_outputs.steam_heater, g_outputs.pump);
}

// =============================================================================
// Public API: Heating Strategy
// =============================================================================

static float calculate_strategy_max_current(uint8_t strategy) {
    electrical_state_t elec_state;
    electrical_state_get(&elec_state);
    
    float brew = elec_state.brew_heater_current;
    float steam = elec_state.steam_heater_current;
    
    switch (strategy) {
        case HEAT_BREW_ONLY: return brew;
        case HEAT_SEQUENTIAL:
        case HEAT_STEAM_PRIORITY: return fmaxf(brew, steam);
        case HEAT_PARALLEL:
        case HEAT_SMART_STAGGER: return brew + steam;
        default: return 0.0f;
    }
}

bool control_is_heating_strategy_allowed(uint8_t strategy) {
    if (strategy > HEAT_SMART_STAGGER) return false;
    
    // Non-dual-boiler machines only support BREW_ONLY (single SSR)
    const machine_features_t* features = machine_get_features();
    if (features && features->type != MACHINE_TYPE_DUAL_BOILER) {
        return (strategy == HEAT_BREW_ONLY);
    }
    
    // Dual boiler: check electrical limits
    electrical_state_t elec_state;
    electrical_state_get(&elec_state);
    
    if (elec_state.nominal_voltage == 0 || elec_state.max_current_draw <= 0.0f) return false;
    
    float max_current = calculate_strategy_max_current(strategy);
    return max_current <= elec_state.max_combined_current;
}

uint8_t control_get_allowed_strategies(uint8_t* allowed, uint8_t max_count) {
    if (!allowed || max_count == 0) return 0;
    
    // Non-dual-boiler machines only support BREW_ONLY
    const machine_features_t* features = machine_get_features();
    if (features && features->type != MACHINE_TYPE_DUAL_BOILER) {
        allowed[0] = HEAT_BREW_ONLY;
        return 1;
    }
    
    // Dual boiler: return all allowed strategies based on electrical limits
    uint8_t count = 0;
    for (uint8_t s = 0; s <= HEAT_SMART_STAGGER && count < max_count; s++) {
        if (control_is_heating_strategy_allowed(s)) {
            allowed[count++] = s;
        }
    }
    return count;
}

bool control_set_heating_strategy(uint8_t strategy) {
    if (strategy > HEAT_SMART_STAGGER) return false;
    
    // Heating strategies only apply to dual boiler machines
    // Single boiler and HX have only one SSR, so only BREW_ONLY is valid
    const machine_features_t* features = machine_get_features();
    if (features && features->type != MACHINE_TYPE_DUAL_BOILER) {
        if (strategy != HEAT_BREW_ONLY) {
            DEBUG_PRINT("Heating strategy: Only BREW_ONLY valid for non-dual-boiler machines\n");
            return false;
        }
    }
    
    if (!control_is_heating_strategy_allowed(strategy)) return false;
    
    g_heating_strategy = (heating_strategy_t)strategy;
    DEBUG_PRINT("Heating strategy: Set to %d\n", strategy);
    return true;
}

uint8_t control_get_heating_strategy(void) {
    return (uint8_t)g_heating_strategy;
}

// =============================================================================
// Public API: Configuration
// =============================================================================

void control_get_config(config_payload_t* config) {
    if (!config) return;
    
    config->brew_setpoint = (int16_t)(g_brew_pid.setpoint * 10);
    config->steam_setpoint = (int16_t)(g_steam_pid.setpoint * 10);
    config->temp_offset = DEFAULT_OFFSET_TEMP;
    config->pid_kp = (uint16_t)(g_brew_pid.kp * 100);
    config->pid_ki = (uint16_t)(g_brew_pid.ki * 100);
    config->pid_kd = (uint16_t)(g_brew_pid.kd * 100);
    config->heating_strategy = control_get_heating_strategy();
    config->machine_type = (uint8_t)machine_get_type();
}

void control_set_config(const config_payload_t* config) {
    if (!config) return;
    
    control_set_setpoint(0, config->brew_setpoint);
    control_set_setpoint(1, config->steam_setpoint);
    control_set_pid(0, config->pid_kp / 100.0f, config->pid_ki / 100.0f, config->pid_kd / 100.0f);
    control_set_heating_strategy(config->heating_strategy);
}

// =============================================================================
// Public API: Single Boiler Mode (delegated to machine-specific)
// =============================================================================

uint8_t control_get_single_boiler_mode(void) {
    return control_get_machine_mode();
}

bool control_is_single_boiler_switching(void) {
    return control_is_machine_switching();
}

