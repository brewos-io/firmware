# Web API Reference

> **Status:** ✅ Implemented  
> **Last Updated:** 2025-12-08  
> **Base URL:** `http://<device-ip>/api`

## Overview

The ESP32 provides a RESTful HTTP API for machine control, configuration, and monitoring. All responses are JSON.

---

## Endpoint Summary

| Method | Path | Description | Status |
|--------|------|-------------|--------|
| GET | `/api/status` | Machine status | ✅ |
| GET | `/api/wifi/networks` | Scan WiFi | ✅ |
| POST | `/api/wifi/connect` | Connect to WiFi | ✅ |
| GET | `/api/config` | Get config | ✅ |
| POST | `/api/command` | Send command | ✅ |
| POST | `/api/ota/upload` | Upload firmware | ✅ |
| POST | `/api/ota/start` | Flash Pico | ✅ |
| POST | `/api/pico/reset` | Reset Pico | ✅ |
| GET | `/api/mqtt/config` | Get MQTT config | ✅ |
| POST | `/api/mqtt/config` | Set MQTT config | ✅ |
| POST | `/api/mqtt/test` | Test MQTT connection | ✅ |
| GET | `/api/schedules` | Get all schedules | ✅ |
| POST | `/api/schedules` | Add schedule | ✅ |
| POST | `/api/schedules/update` | Update schedule | ✅ |
| POST | `/api/schedules/delete` | Delete schedule | ✅ |
| POST | `/api/schedules/toggle` | Enable/disable schedule | ✅ |
| GET | `/api/schedules/auto-off` | Get auto power-off settings | ✅ |
| POST | `/api/schedules/auto-off` | Set auto power-off | ✅ |
| GET | `/api/time` | Get time status & settings | ✅ |
| POST | `/api/time` | Set time/NTP settings | ✅ |
| POST | `/api/time/sync` | Force NTP sync | ✅ |
| GET | `/api/eco/settings` | Get eco mode settings | ✅ |
| POST | `/api/eco/settings` | Set eco mode settings | ✅ |
| GET | `/api/stats` | Get usage statistics | ✅ |
| GET | `/api/stats/extended` | Stats with history | ✅ |
| GET | `/api/preinfusion` | Get pre-infusion settings | ✅ |

---

## Status Endpoints

### GET /api/status

Returns current machine and ESP32 status.

**Response:**
```json
{
  "wifi": {
    "mode": 2,
    "ssid": "MyNetwork",
    "ip": "192.168.1.100",
    "rssi": -45,
    "configured": true
  },
  "pico": {
    "connected": true,
    "packetsReceived": 12345,
    "packetErrors": 0
  },
  "esp32": {
    "uptime": 3600000,
    "freeHeap": 180000,
    "version": "1.0.0"
  },
  "mqtt": {
    "enabled": true,
    "connected": true,
    "status": "Connected"
  },
  "clients": 2
}
```

**WiFi Modes:**
| Value | Mode | Description |
|-------|------|-------------|
| 0 | DISCONNECTED | Not connected |
| 1 | AP_MODE | Access point (setup mode) |
| 2 | STA_MODE | Connected to network |
| 3 | CONNECTING | Connection in progress |

---

## WiFi Endpoints

### GET /api/wifi/networks

Scans for available WiFi networks.

**Response:**
```json
{
  "networks": [
    {
      "ssid": "MyNetwork",
      "rssi": -45,
      "secure": true
    },
    {
      "ssid": "Guest",
      "rssi": -60,
      "secure": false
    }
  ]
}
```

### POST /api/wifi/connect

Connect to a WiFi network.

**Request:**
```json
{
  "ssid": "MyNetwork",
  "password": "mypassword123"
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Connecting to MyNetwork..."
}
```

> **Note:** The device will restart after connecting. The web page will need to be reloaded at the new IP address.

---

## Configuration Endpoints

### GET /api/config

Returns current machine configuration from Pico.

**Response:**
```json
{
  "brew_temp_setpoint": 94.0,
  "steam_temp_setpoint": 145.0,
  "preinfusion_time": 3.0,
  "preinfusion_pressure": 2.0,
  "machine_type": "dual_boiler"
}
```

### POST /api/command

Send a command to the Pico.

**Request:**
```json
{
  "cmd": "set_temp",
  "boiler": "brew",
  "value": 94.0
}
```

**Response:**
```json
{
  "status": "ok"
}
```

---

## OTA Update Endpoints

### POST /api/ota/upload

Upload firmware file for Pico OTA update.

**Request:** `multipart/form-data` with firmware file (.uf2 or .bin)

**Response:** Progress updates via WebSocket:
```json
{
  "type": "ota_progress",
  "stage": "upload",
  "progress": 50,
  "uploaded": 102400,
  "total": 204800
}
```

### POST /api/ota/start

Flash uploaded firmware to Pico.

**Response:** Progress updates via WebSocket:
```json
{
  "type": "ota_progress",
  "stage": "flash",
  "progress": 75,
  "sent": 153600,
  "total": 204800
}
```

### POST /api/pico/reset

Trigger Pico reset via RESET pin.

**Response:**
```json
{
  "status": "ok"
}
```

---

## MQTT Configuration Endpoints

### GET /api/mqtt/config

Get current MQTT configuration.

**Response:**
```json
{
  "enabled": true,
  "broker": "mqtt.example.com",
  "port": 1883,
  "username": "mqtt_user",
  "password": "",
  "client_id": "brewos_xxxxxxxxxxxx",
  "topic_prefix": "brewos",
  "use_tls": false,
  "ha_discovery": true,
  "ha_device_id": "xxxxxxxxxxxx",
  "connected": true,
  "status": "Connected"
}
```

> **Note:** Password is not returned for security.

### POST /api/mqtt/config

Update MQTT configuration.

**Request:**
```json
{
  "enabled": true,
  "broker": "mqtt.example.com",
  "port": 1883,
  "username": "mqtt_user",
  "password": "secret123",
  "topic_prefix": "brewos",
  "ha_discovery": true,
  "ha_device_id": ""
}
```

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `enabled` | boolean | No | false | Enable MQTT |
| `broker` | string | Yes* | - | Broker hostname/IP |
| `port` | integer | No | 1883 | Broker port |
| `username` | string | No | "" | Auth username |
| `password` | string | No | "" | Auth password (only updated if provided) |
| `topic_prefix` | string | No | "brewos" | Topic prefix |
| `ha_discovery` | boolean | No | true | Enable HA discovery |
| `ha_device_id` | string | No | auto | Device ID (auto-generated from MAC if empty) |

**Response:**
```json
{
  "status": "ok"
}
```

### POST /api/mqtt/test

Test MQTT connection with current configuration.

**Response (success):**
```json
{
  "status": "ok",
  "message": "Connection successful"
}
```

**Response (failure):**
```json
{
  "error": "Connection failed"
}
```

---

## WebSocket API

Connect to `/ws` for real-time updates.

### Message Types (Server → Client)

**Status Update:**
```json
{
  "type": "status",
  "brew_temp": 93.5,
  "steam_temp": 145.2,
  "pressure": 9.1,
  "state": 3,
  "state_name": "READY"
}
```

**Log Message:**
```json
{
  "type": "log",
  "message": "WiFi connected",
  "level": "info"
}
```

**Pico Message:**
```json
{
  "type": "pico",
  "msg_type": 1,
  "payload": "base64_encoded_data"
}
```

**OTA Progress:**
```json
{
  "type": "ota_progress",
  "stage": "flash",
  "progress": 50,
  "sent": 102400,
  "total": 204800
}
```

### Message Types (Client → Server)

**Request Status:**
```json
{
  "type": "get_status"
}
```

**Ping:**
```json
{
  "type": "ping"
}
```

---

## Machine Control Endpoints

### POST /api/temp/brew

Set brew boiler temperature.

**Request:**
```json
{
  "temp": 94.0
}
```

**Response:**
```json
{
  "status": "ok"
}
```

### POST /api/temp/steam

Set steam boiler temperature.

**Request:**
```json
{
  "temp": 145.0
}
```

### POST /api/mode

Set machine mode.

**Request:**
```json
{
  "mode": "on"
}
```

| Mode | Description |
|------|-------------|
| `on`, `ready` | Turn machine on |
| `off`, `standby` | Turn machine off |

> **Note:** Brew start/stop is controlled by the physical lever on the machine, not via API.

---

## Scale / Brew-by-Weight Endpoints

### GET /api/scale/status

Get current scale connection status.

**Response:**
```json
{
  "connected": true,
  "scanning": false,
  "name": "Acaia Lunar",
  "type": 1,
  "type_name": "Acaia",
  "weight": 36.5,
  "stable": true,
  "flow_rate": 1.8,
  "battery": 85
}
```

### POST /api/scale/scan

Start BLE scan for scales.

**Response:**
```json
{
  "status": "ok",
  "message": "Scanning..."
}
```

### POST /api/scale/scan/stop

Stop BLE scan.

### GET /api/scale/devices

Get discovered scales from scan.

**Response:**
```json
{
  "devices": [
    {
      "index": 0,
      "name": "Acaia Lunar",
      "address": "AA:BB:CC:DD:EE:FF",
      "type": 1,
      "type_name": "Acaia",
      "rssi": -45
    }
  ],
  "scanning": false,
  "count": 1
}
```

### POST /api/scale/connect

Connect to a scale.

**Request (by address):**
```json
{
  "address": "AA:BB:CC:DD:EE:FF"
}
```

**Request (by index):**
```json
{
  "index": 0
}
```

### POST /api/scale/disconnect

Disconnect from current scale.

### POST /api/scale/forget

Forget saved scale (clear from NVS).

### POST /api/scale/tare

Tare the connected scale.

### GET /api/scale/settings

Get brew-by-weight settings.

**Response:**
```json
{
  "target_weight": 36.0,
  "dose_weight": 18.0,
  "stop_offset": 2.0,
  "auto_stop": true,
  "auto_tare": true
}
```

### POST /api/scale/settings

Update brew-by-weight settings.

**Request:**
```json
{
  "target_weight": 36.0,
  "dose_weight": 18.0,
  "stop_offset": 2.0,
  "auto_stop": true,
  "auto_tare": true
}
```

### GET /api/scale/state

Get current brew-by-weight state.

**Response:**
```json
{
  "active": false,
  "current_weight": 0.0,
  "target_weight": 36.0,
  "progress": 0.0,
  "ratio": 0.0,
  "target_reached": false,
  "stop_signaled": false
}
```

### Scale Timer Control

```
POST /api/scale/timer/start    # Start scale timer (Acaia)
POST /api/scale/timer/stop     # Stop scale timer
POST /api/scale/timer/reset    # Reset scale timer
```

---

## Schedule Endpoints

### GET /api/schedules

Get all schedules and auto power-off settings.

**Response:**
```json
{
  "schedules": [
    {
      "id": 1,
      "enabled": true,
      "days": 62,
      "hour": 7,
      "minute": 0,
      "action": "on",
      "strategy": 1,
      "name": "Morning Coffee"
    }
  ],
  "autoPowerOffEnabled": true,
  "autoPowerOffMinutes": 60
}
```

**Days Bitmask:**
| Bit | Day |
|-----|-----|
| 0x01 | Sunday |
| 0x02 | Monday |
| 0x04 | Tuesday |
| 0x08 | Wednesday |
| 0x10 | Thursday |
| 0x20 | Friday |
| 0x40 | Saturday |

### POST /api/schedules

Add a new schedule.

**Request:**
```json
{
  "enabled": true,
  "days": 62,
  "hour": 7,
  "minute": 0,
  "action": "on",
  "strategy": 1,
  "name": "Morning Coffee"
}
```

**Response:**
```json
{
  "status": "ok",
  "id": 1
}
```

### POST /api/schedules/update

Update an existing schedule.

**Request:**
```json
{
  "id": 1,
  "enabled": true,
  "days": 127,
  "hour": 6,
  "minute": 30,
  "action": "on",
  "strategy": 2,
  "name": "Early Bird"
}
```

### POST /api/schedules/delete

Delete a schedule.

**Request:**
```json
{
  "id": 1
}
```

### POST /api/schedules/toggle

Enable or disable a schedule.

**Request:**
```json
{
  "id": 1,
  "enabled": false
}
```

### GET /api/schedules/auto-off

Get auto power-off settings.

**Response:**
```json
{
  "enabled": true,
  "minutes": 60
}
```

### POST /api/schedules/auto-off

Set auto power-off settings.

**Request:**
```json
{
  "enabled": true,
  "minutes": 90
}
```

---

## Time/NTP Endpoints

### GET /api/time

Get current time status and settings.

**Response:**
```json
{
  "synced": true,
  "currentTime": "2025-11-30 16:30:00",
  "timezone": "UTC+2",
  "utcOffset": 7200,
  "settings": {
    "useNTP": true,
    "ntpServer": "pool.ntp.org",
    "utcOffsetMinutes": 120,
    "dstEnabled": false,
    "dstOffsetMinutes": 60
  }
}
```

### POST /api/time

Update time settings.

**Request:**
```json
{
  "useNTP": true,
  "ntpServer": "pool.ntp.org",
  "utcOffsetMinutes": 120,
  "dstEnabled": true,
  "dstOffsetMinutes": 60
}
```

### POST /api/time/sync

Force NTP synchronization.

**Response:**
```json
{
  "status": "ok",
  "message": "NTP sync initiated"
}
```

---

## Eco Mode Endpoints

### GET /api/eco/settings

Get eco mode configuration.

**Response:**
```json
{
  "enabled": true,
  "brewTemp": 80.0,
  "timeout": 30
}
```

### POST /api/eco/settings

Set eco mode configuration.

**Request:**
```json
{
  "enabled": true,
  "brewTemp": 80.0,
  "timeout": 30
}
```

| Field | Description |
|-------|-------------|
| `enabled` | Eco mode feature enabled |
| `brewTemp` | Temperature (°C) in eco mode |
| `timeout` | Minutes idle before entering eco mode |

---

## Statistics Endpoints

### GET /api/stats

Get full usage statistics.

**Response:**
```json
{
  "lifetime": {
    "totalShots": 1234,
    "totalKwh": 45.67,
    "avgBrewTimeMs": 28500
  },
  "daily": {
    "shotCount": 8,
    "totalKwh": 1.23
  },
  "weekly": { ... },
  "monthly": { ... },
  "maintenance": {
    "shotsSinceBackflush": 45
  }
}
```

### GET /api/stats/extended

Get statistics with all history data (brew history, power samples, daily summaries).

---

## Pre-Infusion Endpoints

### GET /api/preinfusion

Get pre-infusion settings.

**Response:**
```json
{
  "enabled": true,
  "onTimeMs": 2000,
  "pauseTimeMs": 3000
}
```

### POST /api/preinfusion

Set pre-infusion settings (sent via WebSocket command `set_preinfusion`).

---

## Preferences Endpoints

### GET /api/preferences

Get user preferences (included in device_info WebSocket message).

### POST /api/preferences

Set user preferences (sent via WebSocket command `set_preferences`).

**Fields:**
- `firstDayOfWeek`: 0=Sunday, 1=Monday
- `use24HourTime`: boolean
- `temperatureUnit`: 0=celsius, 1=fahrenheit
- `electricityPrice`: float (price per kWh)
- `currency`: string (USD, EUR, GBP, etc.)
- `lastHeatingStrategy`: 0-3

---

## Cloud Integration

```
GET  /api/cloud/status          # Cloud connection status
POST /api/cloud/link            # Link to cloud account
POST /api/cloud/unlink          # Unlink from cloud
```

---

## Authentication (Planned)

```
[ ] API key authentication (header or query param)
[ ] Session-based authentication for web UI
[ ] Rate limiting
[ ] CORS configuration
```

---

## Error Responses

All errors follow this format:

```json
{
  "error": "Error message description"
}
```

| HTTP Code | Description |
|-----------|-------------|
| 400 | Bad Request - Invalid JSON or parameters |
| 404 | Not Found - Endpoint doesn't exist |
| 500 | Server Error - Internal error |

---

## Implementation Details

### Files

| File | Description |
|------|-------------|
| `include/web_server.h` | WebServer class interface |
| `src/web_server.cpp` | HTTP/WebSocket implementation |
| `data/index.html` | Web UI HTML |
| `data/style.css` | Web UI styles |
| `data/app.js` | Web UI JavaScript |

### Dependencies

- `ESPAsyncWebServer` - Async HTTP server
- `AsyncTCP` - Async TCP library
- `ArduinoJson` - JSON parsing/serialization
- `LittleFS` - File system for static files

