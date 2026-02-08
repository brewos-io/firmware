/**
 * Power Meter Manager Implementation (ESP32)
 * 
 * MQTT sources only (v2.32) - Hardware Modbus meters removed from PCB.
 * ESP32 handles MQTT subscription and parsing directly.
 */

#include "power_meter/power_meter_manager.h"
#include "power_meter/mqtt_power_meter.h"
#include "config.h"
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>

// NVS namespace for power meter config
#define NVS_NAMESPACE "power_meter"

PowerMeterManager::PowerMeterManager()
    : _source(PowerMeterSource::NONE)
    , _lastReadTime(0)
    , _mqttMeter(nullptr)
    , _lastPollTime(0)
{
    memset(&_lastReading, 0, sizeof(_lastReading));
}

PowerMeterManager::~PowerMeterManager() {
    cleanupMeter();
}

void PowerMeterManager::begin() {
    LOG_I("Power Meter Manager starting (MQTT-only, v2.32)...");
    
    // Try to load saved configuration
    if (loadConfig()) {
        LOG_I("Loaded power meter config: source=%s", powerMeterSourceToString(_source));
    } else {
        LOG_I("No saved config, defaulting to NONE");
        _source = PowerMeterSource::NONE;
    }
}

void PowerMeterManager::setEnabled(bool enabled) {
    if (enabled != _enabled) {
        _enabled = enabled;
        LOG_I("Power Meter %s", enabled ? "enabled" : "disabled");
    }
}

void PowerMeterManager::loop() {
    // Skip processing if disabled (during OTA)
    if (!_enabled) {
        return;
    }
    
    // Poll MQTT meter if active
    if (_mqttMeter && (millis() - _lastPollTime) >= POLL_INTERVAL_MS) {
        _lastPollTime = millis();
        
        PowerMeterReading reading;
        if (_mqttMeter->read(reading)) {
            _lastReading = reading;
            _lastReadTime = millis();
        }
    }
    
    // Check for daily reset
    time_t now = time(nullptr);
    if (now > 1000000) {  // Time is valid (NTP synced)
        struct tm* tm_now = localtime(&now);
        uint16_t currentDayOfYear = tm_now->tm_yday;
        uint16_t currentYear = tm_now->tm_year + 1900;
        
        // Initialize day start on first valid reading
        if (!_dayStartSet && _lastReading.valid) {
            _dayStartKwh = _lastReading.energy_import;
            _dayStartSet = true;
            _lastDayOfYear = currentDayOfYear;
            _lastYear = currentYear;
            LOG_I("Initialized day start energy: %.3f kWh", _dayStartKwh);
        }
        
        // Reset at midnight (day or year changed)
        if (_dayStartSet && (currentDayOfYear != _lastDayOfYear || currentYear != _lastYear)) {
            resetDailyEnergy();
            _lastDayOfYear = currentDayOfYear;
            _lastYear = currentYear;
        }
    }
}

bool PowerMeterManager::setSource(PowerMeterSource source) {
    if (_source == source) {
        return true;  // No change needed
    }
    
    LOG_I("Changing power meter source: %s -> %s", 
          powerMeterSourceToString(_source),
          powerMeterSourceToString(source));
    
    // Clean up existing meter
    cleanupMeter();
    
    _source = source;
    
    return saveConfig();
}

bool PowerMeterManager::configureMqtt(const char* topic, const char* format) {
    LOG_I("Configuring MQTT power meter: topic=%s, format=%s", topic, format);
    
    // Clean up existing meter
    cleanupMeter();
    
    // Create MQTT meter
    _mqttMeter = new MQTTPowerMeter(topic, format);
    
    if (!_mqttMeter->begin()) {
        LOG_E("Failed to initialize MQTT power meter");
        delete _mqttMeter;
        _mqttMeter = nullptr;
        return false;
    }
    
    LOG_I("MQTT power meter initialized successfully");
    _source = PowerMeterSource::MQTT;
    
    bool saved = saveConfig();
    if (saved) {
        LOG_I("MQTT power meter configuration saved");
    } else {
        LOG_W("Failed to save MQTT power meter configuration");
    }
    
    return saved;
}

const char* PowerMeterManager::getMqttTopic() const {
    if (_source == PowerMeterSource::MQTT && _mqttMeter) {
        return _mqttMeter->getTopic();
    }
    return nullptr;
}

const char* PowerMeterManager::getMqttLwtTopic() const {
    if (_source != PowerMeterSource::MQTT || _mqttMeter == nullptr) {
        return nullptr;
    }
    
    // Auto-derive LWT topic from SENSOR topic:
    //   "tele/tasmota_XXXX/SENSOR" -> "tele/tasmota_XXXX/LWT"
    //   "shellies/shelly-plug-XXX/status" -> "shellies/shelly-plug-XXX/LWT"
    // Strategy: replace the last path segment with "LWT"
    const char* sensorTopic = _mqttMeter->getTopic();
    if (!sensorTopic || sensorTopic[0] == '\0') {
        return nullptr;
    }
    
    const char* lastSlash = strrchr(sensorTopic, '/');
    if (!lastSlash) {
        return nullptr;  // No path separator, can't derive LWT topic
    }
    
    size_t prefixLen = (size_t)(lastSlash - sensorTopic + 1);  // Include the slash
    if (prefixLen + 3 >= sizeof(_lwtTopicBuf)) {
        return nullptr;  // Too long
    }
    
    memcpy(_lwtTopicBuf, sensorTopic, prefixLen);
    memcpy(_lwtTopicBuf + prefixLen, "LWT", 4);  // Includes null terminator
    
    return _lwtTopicBuf;
}

void PowerMeterManager::onMqttPowerMessage(const char* payload, unsigned int length) {
    if (_mqttMeter) {
        _mqttMeter->onMqttData(payload, length);
    }
}

void PowerMeterManager::onMqttLwtMessage(const char* payload, unsigned int length) {
    if (_mqttMeter) {
        _mqttMeter->onLwtMessage(payload, length);
    }
}

bool PowerMeterManager::getReading(PowerMeterReading& reading) {
    // Return last cached reading from MQTT
    reading = _lastReading;
    return _lastReading.valid && ((millis() - _lastReadTime) < 5000);
}

bool PowerMeterManager::isConnected() const {
    if (_source == PowerMeterSource::NONE) {
        return false;
    }
    
    if (_source == PowerMeterSource::MQTT && _mqttMeter) {
        return _mqttMeter->isConnected();
    }
    
    return false;
}

const char* PowerMeterManager::getMeterName() const {
    if (_source == PowerMeterSource::MQTT && _mqttMeter) {
        return _mqttMeter->getName();
    }
    
    return "None";
}

const char* PowerMeterManager::getLastError() const {
    if (_source == PowerMeterSource::NONE) {
        return "No meter configured";
    }
    
    if (_source == PowerMeterSource::MQTT && _mqttMeter) {
        const char* error = _mqttMeter->getLastError();
        return error ? error : "";
    }
    
    return "";
}

void PowerMeterManager::getStatus(JsonDocument& doc) {
    bool connected = isConnected();
    
    doc["source"] = powerMeterSourceToString(_source);
    doc["connected"] = connected;
    doc["meterType"] = getMeterName();
    doc["configured"] = (_source != PowerMeterSource::NONE);
    doc["discovering"] = false;
    
    // Include MQTT topic/format so the UI can pre-fill the config form
    if (_source == PowerMeterSource::MQTT && _mqttMeter) {
        doc["mqttTopic"] = _mqttMeter->getTopic();
        doc["mqttFormat"] = _mqttMeter->getFormat();
    }
    
    if (connected && _lastReading.valid) {
        JsonObject reading = doc["reading"].to<JsonObject>();
        reading["voltage"] = _lastReading.voltage;
        reading["current"] = _lastReading.current;
        reading["power"] = _lastReading.power;
        reading["energy"] = getTodayKwh();                // Today's energy (not lifetime total)
        reading["energyTotal"] = _lastReading.energy_import;  // Lifetime total from meter
        reading["frequency"] = _lastReading.frequency;
        reading["powerFactor"] = _lastReading.power_factor;
        // Convert millis() uptime to Unix epoch ms for the frontend
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        uint64_t nowMs = (uint64_t)tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
        uint32_t ageMs = millis() - _lastReadTime;
        doc["lastUpdate"] = nowMs - ageMs;
    } else {
        doc["reading"] = nullptr;
        doc["lastUpdate"] = nullptr;
    }
    
    const char* error = getLastError();
    if (error && strlen(error) > 0) {
        doc["error"] = error;
    } else {
        doc["error"] = nullptr;
    }
}

bool PowerMeterManager::saveConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("Failed to open NVS for saving");
        return false;
    }
    
    prefs.putUChar("source", (uint8_t)_source);
    
    if (_source == PowerMeterSource::MQTT && _mqttMeter) {
        prefs.putString("mqtt_topic", _mqttMeter->getTopic());
        prefs.putString("mqtt_format", _mqttMeter->getFormat());
    }
    
    prefs.end();
    LOG_I("Power meter config saved");
    return true;
}

bool PowerMeterManager::loadConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    uint8_t sourceVal = prefs.getUChar("source", (uint8_t)PowerMeterSource::NONE);
    
    // Legacy migration: old HARDWARE_MODBUS was enum value 1, but now MQTT is also value 1
    // (since HARDWARE_MODBUS was removed from the enum). Distinguish by checking if an
    // mqtt_topic is saved: if yes, it's a genuine MQTT config; if no, it was old hardware.
    if (sourceVal == 1) {
        String savedTopic = prefs.getString("mqtt_topic", "");
        if (savedTopic.length() == 0) {
            // No MQTT topic saved -> this was old HARDWARE_MODBUS config
            LOG_W("Legacy hardware meter config found, resetting to NONE (hardware removed v2.32)");
            _source = PowerMeterSource::NONE;
            prefs.end();
            return false;
        }
        // Has a topic -> genuine MQTT config
    }
    
    _source = (PowerMeterSource)sourceVal;
    
    if (_source == PowerMeterSource::MQTT) {
        String topic = prefs.getString("mqtt_topic", "");
        String format = prefs.getString("mqtt_format", "auto");
        if (topic.length() > 0) {
            prefs.end();
            return configureMqtt(topic.c_str(), format.c_str());
        }
    }
    
    prefs.end();
    return _source != PowerMeterSource::NONE;
}

void PowerMeterManager::cleanupMeter() {
    if (_mqttMeter) {
        delete _mqttMeter;
        _mqttMeter = nullptr;
    }
}

float PowerMeterManager::getTodayKwh() const {
    if (!_lastReading.valid || !_dayStartSet) {
        return 0.0f;
    }
    
    // Today's energy = current reading - day start reading
    float today = _lastReading.energy_import - _dayStartKwh;
    
    // Handle meter reset or rollover
    if (today < 0.0f) {
        today = _lastReading.energy_import;  // Meter was reset, use current value
    }
    
    return today;
}

void PowerMeterManager::resetDailyEnergy() {
    // Call at midnight to reset daily tracking
    _dayStartKwh = _lastReading.valid ? _lastReading.energy_import : 0.0f;
    _dayStartSet = true;
    LOG_I("Daily energy reset: day start = %.3f kWh", _dayStartKwh);
}

// Global instance - now a pointer, constructed in main.cpp setup()
PowerMeterManager* powerMeterManager = nullptr;
