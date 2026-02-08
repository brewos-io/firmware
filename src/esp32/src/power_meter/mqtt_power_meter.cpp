/**
 * MQTT Power Meter Implementation
 */

#include "power_meter/mqtt_power_meter.h"
#include "config.h"

MQTTPowerMeter::MQTTPowerMeter(const char* topic, const char* format)
    : _topic(topic)
    , _lastUpdateTime(0)
    , _hasData(false)
    , _deviceOnline(true)  // Assume online until LWT says otherwise
{
    _lastError[0] = '\0';
    memset(&_lastReading, 0, sizeof(_lastReading));
    
    // Parse format
    if (strcmp(format, "shelly") == 0) {
        _format = MqttFormat::SHELLY;
    } else if (strcmp(format, "tasmota") == 0) {
        _format = MqttFormat::TASMOTA;
    } else if (strcmp(format, "generic") == 0) {
        _format = MqttFormat::GENERIC;
    } else {
        _format = MqttFormat::AUTO;
    }
}

MQTTPowerMeter::~MQTTPowerMeter() {
    // Nothing to clean up
}

bool MQTTPowerMeter::begin() {
    LOG_I("MQTT power meter initialized: topic=%s, format=%s", 
          _topic.c_str(), getFormat());
    return true;
}

void MQTTPowerMeter::loop() {
    // No polling needed - data comes via callback
}

bool MQTTPowerMeter::read(PowerMeterReading& reading) {
    if (!isConnected()) {
        return false;
    }
    
    reading = _lastReading;
    return true;
}

const char* MQTTPowerMeter::getName() const {
    return _topic.c_str();
}

bool MQTTPowerMeter::isConnected() const {
    // Connected if:
    //   - We have received data at least once, AND
    //   - Either the device is online (per LWT), or data is fresh (per stale threshold)
    // LWT is the primary indicator. The stale threshold is a generous fallback
    // for devices that don't publish LWT (e.g. Shelly with custom topics).
    return _hasData && (_deviceOnline || !isStale());
}

const char* MQTTPowerMeter::getFormat() const {
    switch (_format) {
        case MqttFormat::SHELLY: return "shelly";
        case MqttFormat::TASMOTA: return "tasmota";
        case MqttFormat::GENERIC: return "generic";
        case MqttFormat::AUTO: return "auto";
        default: return "unknown";
    }
}

bool MQTTPowerMeter::isStale() const {
    return (millis() - _lastUpdateTime) > STALE_THRESHOLD_MS;
}

void MQTTPowerMeter::setJsonPaths(const char* power, const char* voltage, 
                                  const char* current, const char* energy) {
    _jsonPathPower = power ? power : "";
    _jsonPathVoltage = voltage ? voltage : "";
    _jsonPathCurrent = current ? current : "";
    _jsonPathEnergy = energy ? energy : "";
    _format = MqttFormat::GENERIC;
}

void MQTTPowerMeter::onLwtMessage(const char* payload, size_t length) {
    // Tasmota LWT: "Online" or "Offline"
    // Shelly LWT: "true" or "false" (some models)
    bool wasOnline = _deviceOnline;
    
    if (length >= 6 && strncasecmp(payload, "Online", 6) == 0) {
        _deviceOnline = true;
    } else if (length >= 7 && strncasecmp(payload, "Offline", 7) == 0) {
        _deviceOnline = false;
    } else if (length >= 4 && strncasecmp(payload, "true", 4) == 0) {
        _deviceOnline = true;
    } else if (length >= 5 && strncasecmp(payload, "false", 5) == 0) {
        _deviceOnline = false;
    } else {
        // Unknown payload, ignore
        return;
    }
    
    if (wasOnline != _deviceOnline) {
        LOG_I("MQTT power meter device %s (LWT)", _deviceOnline ? "online" : "offline");
    }
}

void MQTTPowerMeter::onMqttData(const char* payload, size_t length) {
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        snprintf(_lastError, sizeof(_lastError), "JSON parse error: %s", error.c_str());
        LOG_W("MQTT power meter JSON parse error: %s", error.c_str());
        return;
    }
    
    onMqttData(doc);
}

void MQTTPowerMeter::onMqttData(JsonDocument& doc) {
    bool parsed = false;
    
    // Log connection status changes
    static bool wasConnected = false;
    
    // Try format-specific parsing
    switch (_format) {
        case MqttFormat::SHELLY:
            parsed = parseShelly(doc);
            break;
        case MqttFormat::TASMOTA:
            parsed = parseTasmota(doc);
            break;
        case MqttFormat::GENERIC:
            parsed = parseGeneric(doc);
            break;
        case MqttFormat::AUTO:
            parsed = tryAutoParse(doc);
            break;
    }
    
    if (parsed) {
        _lastReading.timestamp = millis();
        _lastReading.valid = true;
        _lastUpdateTime = millis();
        bool isNowConnected = true;
        if (!wasConnected) {
            LOG_I("MQTT power meter connected: topic=%s, format=%s", _topic.c_str(), getFormat());
        }
        _hasData = true;
        _lastError[0] = '\0';
        wasConnected = isNowConnected;
    } else {
        snprintf(_lastError, sizeof(_lastError), "Failed to parse MQTT data");
        if (wasConnected) {
            LOG_W("MQTT power meter data parse failed: topic=%s", _topic.c_str());
        }
        wasConnected = false;
    }
}

bool MQTTPowerMeter::parseShelly(JsonDocument& doc) {
    // Shelly Plug format:
    // {
    //   "wifi_sta": {...},
    //   "cloud": {...},
    //   "mqtt": {...},
    //   "time": "...",
    //   "serial": ...,
    //   "has_update": false,
    //   "mac": "...",
    //   "relays": [{...}],
    //   "meters": [{
    //     "power": 123.45,
    //     "overpower": 0,
    //     "is_valid": true,
    //     "timestamp": ...,
    //     "counters": [1234, 5678, ...],
    //     "total": 1234
    //   }],
    //   "temperature": 25.67,
    //   "overtemperature": false,
    //   "tmp": {...}
    // }
    
    if (doc["meters"].is<JsonArray>()) {
        JsonArray meters = doc["meters"].as<JsonArray>();
        if (meters.size() > 0) {
            JsonObject meter = meters[0].as<JsonObject>();
            
            if (meter["power"].is<float>()) {
                _lastReading.power = meter["power"].as<float>();
            }
            if (meter["total"].is<float>()) {
                // Convert Watt-minutes to kWh: Wm → Wh (/60) → kWh (/1000)
                _lastReading.energy_import = meter["total"].as<float>() / 60.0f / 1000.0f;
            }
            
            // Shelly doesn't provide voltage/current in status, estimate from power
            // Assume 230V for European plugs
            _lastReading.voltage = 230.0f;
            if (_lastReading.power > 0 && _lastReading.voltage > 0) {
                _lastReading.current = _lastReading.power / _lastReading.voltage;
            }
            
            return true;
        }
    }
    
    return false;
}

bool MQTTPowerMeter::parseTasmota(JsonDocument& doc) {
    // Tasmota SENSOR format:
    // {
    //   "Time": "2024-01-01T00:00:00",
    //   "ENERGY": {
    //     "TotalStartTime": "2024-01-01T00:00:00",
    //     "Total": 12.345,
    //     "Yesterday": 1.234,
    //     "Today": 0.123,
    //     "Power": 123,
    //     "ApparentPower": 125,
    //     "ReactivePower": 20,
    //     "Factor": 0.98,
    //     "Voltage": 230,
    //     "Current": 0.536
    //   }
    // }
    
    if (doc["ENERGY"].is<JsonObject>()) {
        JsonObject energy = doc["ENERGY"].as<JsonObject>();
        
        // Tasmota may send Power/Voltage/Current/Total/Factor as int or float
        if (energy["Power"].is<float>() || energy["Power"].is<int>()) {
            _lastReading.power = energy["Power"].as<float>();
        }
        if (energy["Voltage"].is<float>() || energy["Voltage"].is<int>()) {
            _lastReading.voltage = energy["Voltage"].as<float>();
        }
        if (energy["Current"].is<float>() || energy["Current"].is<int>()) {
            _lastReading.current = energy["Current"].as<float>();
        }
        if (energy["Total"].is<float>() || energy["Total"].is<int>()) {
            _lastReading.energy_import = energy["Total"].as<float>();
        }
        if (energy["Factor"].is<float>() || energy["Factor"].is<int>()) {
            _lastReading.power_factor = energy["Factor"].as<float>();
        }
        
        _lastReading.frequency = 50.0f;  // Assume 50Hz
        
        return true;
    }
    
    return false;
}

bool MQTTPowerMeter::parseGeneric(JsonDocument& doc) {
    bool success = false;
    
    if (_jsonPathPower.length() > 0) {
        success |= extractJsonValue(doc, _jsonPathPower, _lastReading.power);
    }
    if (_jsonPathVoltage.length() > 0) {
        success |= extractJsonValue(doc, _jsonPathVoltage, _lastReading.voltage);
    }
    if (_jsonPathCurrent.length() > 0) {
        success |= extractJsonValue(doc, _jsonPathCurrent, _lastReading.current);
    }
    if (_jsonPathEnergy.length() > 0) {
        success |= extractJsonValue(doc, _jsonPathEnergy, _lastReading.energy_import);
    }
    
    return success;
}

bool MQTTPowerMeter::tryAutoParse(JsonDocument& doc) {
    // Try Shelly format first
    if (parseShelly(doc)) {
        _format = MqttFormat::SHELLY;
        LOG_I("Auto-detected Shelly format");
        return true;
    }
    
    // Try Tasmota format
    if (parseTasmota(doc)) {
        _format = MqttFormat::TASMOTA;
        LOG_I("Auto-detected Tasmota format");
        return true;
    }
    
    // Try simple direct fields
    bool found = false;
    if (doc["power"].is<float>()) {
        _lastReading.power = doc["power"].as<float>();
        found = true;
    }
    if (doc["voltage"].is<float>()) {
        _lastReading.voltage = doc["voltage"].as<float>();
        found = true;
    }
    if (doc["current"].is<float>()) {
        _lastReading.current = doc["current"].as<float>();
        found = true;
    }
    if (doc["energy"].is<float>()) {
        _lastReading.energy_import = doc["energy"].as<float>();
        found = true;
    }
    
    if (found) {
        LOG_I("Auto-detected simple JSON format");
        return true;
    }
    
    return false;
}

bool MQTTPowerMeter::extractJsonValue(JsonDocument& doc, const String& path, float& value) {
    if (path.length() == 0) {
        return false;
    }
    
    // Simple path support (no nested paths for now - just top-level keys)
    if (doc[path.c_str()].is<float>()) {
        value = doc[path.c_str()].as<float>();
        return true;
    }
    
    return false;
}

