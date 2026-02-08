/**
 * Power Meter Manager (ESP32)
 * 
 * Manages power meter data from MQTT sources (Shelly, Tasmota, generic smart plugs).
 * Hardware meters (PZEM, JSY, Eastron) removed in v2.32.
 */

#ifndef POWER_METER_MANAGER_H
#define POWER_METER_MANAGER_H

#include "power_meter.h"
#include <ArduinoJson.h>

// Forward declarations
class MQTTPowerMeter;

class PowerMeterManager {
public:
    PowerMeterManager();
    ~PowerMeterManager();
    
    // Lifecycle
    void begin();
    void loop();
    
    /**
     * Enable/disable power meter polling (for OTA updates)
     * When disabled, stops all HTTP/MQTT polling
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }
    
    // Configuration
    bool setSource(PowerMeterSource source);
    bool configureMqtt(const char* topic, const char* format = "auto");
    PowerMeterSource getSource() const { return _source; }
    
    /**
     * MQTT power meter: topic to subscribe to (when source is MQTT).
     * Returns nullptr if not configured for MQTT.
     */
    const char* getMqttTopic() const;
    
    /**
     * MQTT power meter: auto-derived LWT topic (e.g. tele/device/LWT from tele/device/SENSOR).
     * Returns nullptr if not configured for MQTT or topic can't be derived.
     */
    const char* getMqttLwtTopic() const;
    
    /**
     * MQTT power meter: called when a message arrives on the configured topic.
     * Forwards payload to the internal MQTTPowerMeter parser.
     */
    void onMqttPowerMessage(const char* payload, unsigned int length);
    
    /**
     * MQTT power meter: called when a LWT message arrives (Online/Offline).
     * Forwards to the internal MQTTPowerMeter.
     */
    void onMqttLwtMessage(const char* payload, unsigned int length);
    
    // Data access
    bool getReading(PowerMeterReading& reading);
    bool isConnected() const;
    const char* getMeterName() const;
    const char* getLastError() const;
    
    // Energy tracking - daily vs total
    float getDayStartKwh() const { return _dayStartKwh; }
    float getTodayKwh() const;
    float getTotalKwh() const { return _lastReading.energy_import; }
    void resetDailyEnergy();  // Call at midnight
    
    // Status for WebSocket/MQTT publishing
    void getStatus(JsonDocument& doc);
    
    // Save/load configuration
    bool saveConfig();
    bool loadConfig();
    
private:
    PowerMeterSource _source;
    PowerMeterReading _lastReading;
    uint32_t _lastReadTime;
    
    // MQTT meter (only supported source since v2.32)
    MQTTPowerMeter* _mqttMeter;
    
    // Auto-derived LWT topic buffer (e.g. "tele/device/LWT" from "tele/device/SENSOR")
    mutable char _lwtTopicBuf[128];
    
    // Helper methods
    void cleanupMeter();
    
    // Polling interval (don't query too frequently)
    static constexpr uint32_t POLL_INTERVAL_MS = 1000;
    uint32_t _lastPollTime;
    
    // Daily energy tracking
    float _dayStartKwh = 0.0f;   // Energy reading at start of current day
    uint16_t _lastDayOfYear = 0; // Track day changes (0-365)
    uint16_t _lastYear = 0;      // Track year for rollover handling
    bool _dayStartSet = false;   // True once we've captured day start
    
    // OTA pause flag
    bool _enabled = true;
};

// Global instance
extern PowerMeterManager* powerMeterManager;

#endif // POWER_METER_MANAGER_H

