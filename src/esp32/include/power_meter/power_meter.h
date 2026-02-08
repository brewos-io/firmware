/**
 * Universal Power Meter Interface
 * 
 * Supports MQTT sources (Shelly, Tasmota, generic smart plugs)
 * Hardware Modbus meters removed in v2.32
 */

#ifndef POWER_METER_H
#define POWER_METER_H

#include <Arduino.h>

// Unified data model for all power meters
struct PowerMeterReading {
    float voltage;        // Volts (RMS)
    float current;        // Amps (RMS)
    float power;          // Watts (Active)
    float energy_import;  // kWh (from grid)
    float energy_export;  // kWh (to grid - for solar/bidirectional)
    float frequency;      // Hz
    float power_factor;   // 0.0 - 1.0
    uint32_t timestamp;   // millis() when read
    bool valid;           // Reading successful
};

// Power meter source types
enum class PowerMeterSource {
    NONE,
    // HARDWARE_MODBUS removed (v2.32) - hardware power metering eliminated from PCB
    MQTT              // MQTT topic subscription (only supported source)
};

// Abstract base class for all power meters
class IPowerMeter {
public:
    virtual ~IPowerMeter() = default;
    
    // Initialize the meter (hardware setup, subscriptions, etc.)
    virtual bool begin() = 0;
    
    // Update loop - called frequently for polling/processing
    virtual void loop() = 0;
    
    // Read current power data
    virtual bool read(PowerMeterReading& reading) = 0;
    
    // Get meter identification
    virtual const char* getName() const = 0;
    
    // Get source type
    virtual PowerMeterSource getSource() const = 0;
    
    // Check if meter is connected/responding
    virtual bool isConnected() const = 0;
    
    // Get last error message (if any)
    virtual const char* getLastError() const { return nullptr; }
};

// Helper function to convert source enum to string
inline const char* powerMeterSourceToString(PowerMeterSource source) {
    switch (source) {
        case PowerMeterSource::NONE: return "none";
        case PowerMeterSource::MQTT: return "mqtt";
        default: return "unknown";
    }
}

// Helper function to parse source string to enum
inline PowerMeterSource stringToPowerMeterSource(const char* str) {
    if (strcmp(str, "mqtt") == 0) return PowerMeterSource::MQTT;
    // Legacy: treat "hardware" as NONE since hardware metering was removed
    if (strcmp(str, "hardware") == 0) return PowerMeterSource::NONE;
    return PowerMeterSource::NONE;
}

#endif // POWER_METER_H

