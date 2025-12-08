# MQTT Integration

> **Status:** ✅ Implemented  
> **Last Updated:** 2025-12-08

## Overview

MQTT provides lightweight pub/sub messaging for home automation integration (Home Assistant, Node-RED, etc.). The ESP32 acts as an MQTT client, publishing machine status and subscribing to commands.

For a complete Home Assistant integration including custom Lovelace card and native HA integration, see [homeassistant/README.md](../../../homeassistant/README.md).

## Configuration

### Web UI

Configure MQTT via the web interface at `http://<device-ip>/` → Settings → Network → MQTT.

### NVS Storage

Configuration is persisted in ESP32 NVS (Non-Volatile Storage):

```cpp
struct MQTTConfig {
    bool enabled;           // Enable/disable MQTT
    char broker[64];        // Broker hostname or IP
    uint16_t port;          // Broker port (default: 1883)
    char username[32];      // Authentication username (optional)
    char password[64];      // Authentication password (optional)
    char client_id[32];     // MQTT client ID (auto-generated if empty)
    char topic_prefix[32];  // Topic prefix (default: "brewos")
    bool use_tls;           // Enable TLS/SSL (not yet implemented)
    bool ha_discovery;      // Enable Home Assistant auto-discovery
    char ha_device_id[32];  // Device ID for topics (auto-generated from MAC)
};
```

---

## Topic Structure

All topics follow the pattern: `{topic_prefix}/{device_id}/{topic_name}`

Default prefix is `brewos`, device_id is derived from MAC address.

| Topic | Direction | QoS | Retained | Description |
|-------|-----------|-----|----------|-------------|
| `brewos/{id}/availability` | Publish | 1 | Yes | Online/offline status (LWT) |
| `brewos/{id}/status` | Publish | 0 | Yes | Full machine status JSON |
| `brewos/{id}/power` | Publish | 0 | Yes | Power meter readings |
| `brewos/{id}/statistics` | Publish | 0 | Yes | Shot/energy statistics |
| `brewos/{id}/shot` | Publish | 0 | No | Shot data when brewing completes |
| `brewos/{id}/command` | Subscribe | 1 | No | Incoming commands |

---

## Status Payload

Published every 1 second to `brewos/{id}/status`:

```json
{
  "state": "ready",
  "mode": "on",
  "heating_strategy": 1,
  "brew_temp": 93.5,
  "brew_setpoint": 94.0,
  "steam_temp": 145.2,
  "steam_setpoint": 145.0,
  "pressure": 9.1,
  "scale_weight": 0.0,
  "flow_rate": 0.0,
  "shot_duration": 0.0,
  "shot_weight": 0.0,
  "target_weight": 36.0,
  "is_brewing": false,
  "is_heating": true,
  "water_low": false,
  "alarm_active": false,
  "pico_connected": true,
  "wifi_connected": true,
  "scale_connected": false
}
```

### Machine States

| Value | State | Description |
|-------|-------|-------------|
| `standby` | Standby | Machine off/idle |
| `heating` | Heating | Heating to setpoint |
| `ready` | Ready | At temperature, ready to brew |
| `brewing` | Brewing | Actively brewing |
| `steaming` | Steaming | Steam wand active |
| `cooldown` | Cooldown | Cooling down |
| `fault` | Fault | Error condition |

### Machine Modes

| Value | Mode | Description |
|-------|------|-------------|
| `standby` | Standby | Machine powered off |
| `on` | On | Machine running |
| `eco` | Eco | Energy-saving mode |

### Heating Strategies

| Value | Strategy | Description |
|-------|----------|-------------|
| 0 | Brew Only | Heat brew boiler only |
| 1 | Sequential | Heat brew first, then steam |
| 2 | Parallel | Heat both boilers simultaneously |
| 3 | Smart Stagger | Intelligent staggered heating |

---

## Statistics Payload

Published to `brewos/{id}/statistics` when statistics change:

```json
{
  "shots_today": 5,
  "total_shots": 1234,
  "kwh_today": 1.25
}
```

---

## Power Meter Payload

Published every 5 seconds to `brewos/{id}/power` when power metering is enabled:

```json
{
  "voltage": 230.5,
  "current": 5.2,
  "power": 1198,
  "energy_import": 45.3,
  "energy_export": 0.0,
  "frequency": 50.0,
  "power_factor": 0.98
}
```

---

## Command Payloads

Subscribe to `brewos/{id}/command` to send commands:

### Set Mode

```json
{
  "cmd": "set_mode",
  "mode": "on",
  "strategy": 1
}
```

| Field | Type | Values |
|-------|------|--------|
| `cmd` | string | `"set_mode"` |
| `mode` | string | `"standby"`, `"on"`, `"eco"` |
| `strategy` | int | 0-3 (optional, only for mode "on") |

### Set Temperature

```json
{
  "cmd": "set_temp",
  "boiler": "brew",
  "temp": 94.0
}
```

| Field | Type | Values |
|-------|------|--------|
| `cmd` | string | `"set_temp"` |
| `boiler` | string | `"brew"` or `"steam"` |
| `temp` | float | Temperature in °C |

### Set Heating Strategy

```json
{
  "cmd": "set_heating_strategy",
  "strategy": 1
}
```

| Field | Type | Values |
|-------|------|--------|
| `cmd` | string | `"set_heating_strategy"` |
| `strategy` | int | 0=brew_only, 1=sequential, 2=parallel, 3=smart_stagger |

### Set Target Weight (BBW)

```json
{
  "cmd": "set_target_weight",
  "weight": 36.0
}
```

### Brew Control

```json
{"cmd": "brew_start"}
{"cmd": "brew_stop"}
```

### Scale Control

```json
{"cmd": "tare"}
```

### Eco Mode Control

```json
{"cmd": "enter_eco"}
{"cmd": "exit_eco"}
```

### Set Eco Configuration

```json
{
  "cmd": "set_eco",
  "enabled": true,
  "brewTemp": 80.0,
  "timeout": 30
}
```

---

## Home Assistant Integration

### Auto-Discovery

When `ha_discovery` is enabled, the ESP32 publishes discovery messages to Home Assistant's MQTT discovery topic. Over 35 entities are automatically created.

### Discovered Entities

**Sensors:**

| Entity ID | Description | Unit |
|-----------|-------------|------|
| `sensor.brewos_brew_temp` | Brew boiler temperature | °C |
| `sensor.brewos_steam_temp` | Steam boiler temperature | °C |
| `sensor.brewos_brew_setpoint` | Brew target temperature | °C |
| `sensor.brewos_steam_setpoint` | Steam target temperature | °C |
| `sensor.brewos_pressure` | Brew pressure | bar |
| `sensor.brewos_scale_weight` | Scale weight | g |
| `sensor.brewos_flow_rate` | Flow rate | g/s |
| `sensor.brewos_shot_duration` | Shot timer | s |
| `sensor.brewos_shot_weight` | Shot weight | g |
| `sensor.brewos_target_weight` | BBW target weight | g |
| `sensor.brewos_shots_today` | Daily shot count | shots |
| `sensor.brewos_total_shots` | Lifetime shots | shots |
| `sensor.brewos_energy_today` | Daily energy | kWh |
| `sensor.brewos_power` | Current power | W |
| `sensor.brewos_voltage` | Line voltage | V |
| `sensor.brewos_current` | Current draw | A |

**Binary Sensors:**

| Entity ID | Device Class |
|-----------|--------------|
| `binary_sensor.brewos_is_brewing` | running |
| `binary_sensor.brewos_is_heating` | heat |
| `binary_sensor.brewos_ready` | - |
| `binary_sensor.brewos_water_low` | problem |
| `binary_sensor.brewos_alarm_active` | problem |
| `binary_sensor.brewos_pico_connected` | connectivity |
| `binary_sensor.brewos_scale_connected` | connectivity |

**Controls:**

| Entity ID | Type | Description |
|-----------|------|-------------|
| `switch.brewos_power_switch` | Switch | Machine power on/off |
| `button.brewos_start_brew` | Button | Start brewing |
| `button.brewos_stop_brew` | Button | Stop brewing |
| `button.brewos_tare_scale` | Button | Tare scale |
| `button.brewos_enter_eco` | Button | Enter eco mode |
| `button.brewos_exit_eco` | Button | Exit eco mode |
| `number.brewos_brew_temp_target` | Number | Brew temperature (85-100°C) |
| `number.brewos_steam_temp_target` | Number | Steam temperature (120-160°C) |
| `number.brewos_bbw_target` | Number | Target weight (18-100g) |
| `select.brewos_mode_select` | Select | Mode: standby, on, eco |
| `select.brewos_heating_strategy` | Select | Heating strategy |

### Home Assistant Configuration

No manual configuration needed! Entities appear automatically when:
1. MQTT is enabled and connected
2. Home Assistant Discovery is enabled
3. HA is connected to the same MQTT broker

For additional features (custom card, automations, dashboard), see [homeassistant/README.md](../../../homeassistant/README.md).

---

## Connection Management

### Auto-Reconnect

- Exponential backoff starting at 1 second
- Maximum delay: 60 seconds
- Resets to 1 second on successful connection

### Last Will Testament (LWT)

- Topic: `brewos/{id}/availability`
- Payload: `"offline"`
- QoS: 1
- Retained: Yes

When the ESP32 connects, it publishes `"online"` to the same topic.

### Keep-Alive

- Interval: 15 seconds (PubSubClient default)
- Clean session: Yes

---

## Implementation Details

### Files

| File | Description |
|------|-------------|
| `include/mqtt_client.h` | MQTTClient class interface |
| `src/mqtt_client.cpp` | Implementation |

### Dependencies

- `PubSubClient` - MQTT client library
- `ArduinoJson` - JSON serialization

### Buffer Size

MQTT buffer is set to 1024 bytes to accommodate Home Assistant discovery payloads.

---

## Troubleshooting

### Connection Failed

Check:
1. Broker address and port are correct
2. Username/password if authentication is required
3. WiFi is connected
4. Firewall allows MQTT traffic (port 1883 or 8883 for TLS)

### Entities Not Appearing in Home Assistant

Check:
1. HA Discovery is enabled in MQTT settings
2. Home Assistant is connected to the same MQTT broker
3. Check MQTT Explorer for discovery messages at `homeassistant/sensor/brewos_*`

### Status Not Updating

Check:
1. MQTT is connected (check status in web UI)
2. Status topic is being published (use MQTT Explorer)
3. Check ESP32 logs for publish errors

---

## See Also

- [Home Assistant Integration](../../../homeassistant/README.md) - Full HA integration with custom card
- [Power Metering Guide](../../web/Power_Metering.md) - Power monitoring setup
- [WebSocket Protocol](../../web/WebSocket_Protocol.md) - Real-time web interface protocol
