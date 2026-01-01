#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Sensor Data Structure
// -----------------------------------------------------------------------------
// Optimized member order (largest to smallest) to minimize padding:
// int16/uint16 (2 bytes) -> uint8 (1 byte)
typedef struct {
    int16_t brew_temp;      // Celsius * 10 (0.1C resolution)
    int16_t steam_temp;     // Celsius * 10
    int16_t group_temp;     // Celsius * 10
    uint16_t pressure;      // Bar * 100 (0.01 bar resolution)
    uint8_t water_level;    // 0-100%
    // 1 byte padding may be added by compiler for alignment
    // Total size: 10 bytes (with padding) or 9 bytes (packed)
} sensor_data_t;

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

// Initialize all sensors
void sensors_init(void);

// Read all sensors (call periodically)
void sensors_read(void);

// Get all sensor data
void sensors_get_data(sensor_data_t* data);

// Get individual values
int16_t sensors_get_brew_temp(void);
int16_t sensors_get_steam_temp(void);
int16_t sensors_get_group_temp(void);
uint16_t sensors_get_pressure(void);
uint8_t sensors_get_water_level(void);

// Simulation control (for development without hardware)
void sensors_set_simulation(bool enable);
void sensors_sim_set_heating(bool heating);

#endif // SENSORS_H

