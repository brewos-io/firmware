# ESP32 Implementation Plan

> **Status:** Active Development  
> **Last Updated:** 2025-12-22

## Overview

The ESP32-S3 serves as the connectivity and UI hub for the BrewOS coffee machine controller. It bridges the Raspberry Pi Pico (which handles real-time control) with the user, providing WiFi, web interface, MQTT, cloud integration, and brew-by-weight features.

---

## Implementation Status

### Core Features

| Feature             | Status      | Notes                                                   |
| ------------------- | ----------- | ------------------------------------------------------- |
| WiFi AP Mode        | ✅ Complete | `BrewOS-Setup` access point                             |
| WiFi STA Mode       | ✅ Complete | Connect to home network                                 |
| UART Bridge to Pico | ✅ Complete | 921600 baud, CRC-16 packets                             |
| Basic Web Server    | ✅ Complete | LittleFS static files                                   |
| WebSocket Status    | ✅ Complete | Real-time status updates                                |
| OTA Pico Update     | ✅ Complete | Firmware streaming                                      |
| Basic Dashboard UI  | ✅ Complete | Temperature, pressure display                           |
| MQTT Integration    | ✅ Complete | [Details](integrations/MQTT.md)                         |
| Web API             | ✅ Complete | [Details](integrations/Web_API.md)                      |
| LVGL Display        | ✅ Complete | Round display UI                                        |
| BLE Scale           | ✅ Complete | Multi-scale support (Acaia, Felicita, Decent, Timemore) |
| Brew by Weight      | ✅ Complete | Auto-stop at target weight                              |
| Cloud Integration   | ✅ Complete | [Details](https://github.com/brewos-io/cloud/blob/main/docs/README.md) |

---

## 1. Hardware Integration

### Target Device

**Model:** UEDX48480021-MD80E (ESP32-S3 Knob Display)

| Specification | Value                        |
| ------------- | ---------------------------- |
| Screen        | 2.1" Round IPS, 480×480      |
| MCU           | ESP32-S3                     |
| RAM           | 8 MB PSRAM                   |
| Flash         | 16 MB                        |
| Input         | Rotary Encoder + Push Button |
| Graphics      | LVGL                         |

### Pin Configuration

| GPIO | Function          | Status |
| ---- | ----------------- | ------ |
| 17   | UART TX → Pico RX | ✅     |
| 18   | UART RX ← Pico TX | ✅     |
| 8    | PICO_RUN (Reset)  | ✅     |
| 9    | PICO_BOOTSEL      | ✅     |
| 10   | WEIGHT_STOP       | ✅     |
| 14   | Encoder CLK       | ✅     |
| 13   | Encoder DT        | ✅     |
| 15   | Encoder SW        | ✅     |

### Tasks

```
[x] HW-1: Configure LVGL for 480x480 round display
[x] HW-2: Initialize display driver
[x] HW-3: Implement rotary encoder with debouncing
[x] HW-4: Button press detection (short/long/double)
[x] HW-5: Backlight PWM control
[ ] HW-6: Display sleep mode
[ ] HW-7: ESP32 OTA self-update
[ ] HW-8: Hardware watchdog
```

---

## 2. Display UI (LVGL)

Round display screens with rotary encoder navigation.

**👉 See [UI Design](UI_Design.md) for complete screen specifications.**

### Screens

| Screen        | Description                | Status      |
| ------------- | -------------------------- | ----------- |
| Setup         | WiFi AP info               | ✅ Complete |
| Idle          | Turn on, heating strategy  | ✅ Complete |
| Home          | Brew/Steam temps, pressure | ✅ Complete |
| Brewing       | Timer, weight, flow        | ✅ Complete |
| Shot Complete | Summary, save option       | ✅ Complete |
| Settings      | Menu navigation            | ✅ Complete |
| Temperature   | Setpoint adjustment        | ✅ Complete |
| Scale Pairing | BLE scale connection       | ✅ Complete |
| Alarm         | Error display              | ✅ Complete |

### Tasks

```
[x] UI-1: Setup screen with AP info
[x] UI-2: Idle screen with heating strategy
[x] UI-3: Home screen with temperature arcs
[x] UI-4: Brewing screen with timer/weight
[x] UI-5: Settings menu navigation
[x] UI-6: Scale pairing screen
[x] UI-7: Alarm handling
[x] UI-8: Screen transitions and animations
```

---

## 3. MQTT Integration

**Status:** ✅ Complete

See [MQTT Integration](integrations/MQTT.md) for full documentation.

### Summary

- PubSubClient library
- Home Assistant auto-discovery
- Status publishing (1s interval)
- Command subscription
- Exponential backoff reconnect

---

## 4. Web API

**Status:** ✅ Complete

See [Web API Reference](integrations/Web_API.md) for full documentation.

### Summary

- RESTful HTTP endpoints
- WebSocket real-time updates
- MQTT configuration
- OTA firmware upload

---

## 5. Cloud Integration

**Status:** ✅ Complete

See [Cloud Service Documentation](https://github.com/brewos-io/cloud/blob/main/docs/README.md) for full details.

### Summary

- WebSocket relay through cloud.brewos.io
- Google OAuth for user authentication
- QR code device pairing
- Same UI and functionality as local access
- Stateless relay design (all data stays on ESP32)

---

## 6. Brew by Weight (BLE Scale)

**Status:** ✅ Complete

### Supported Scales

| Scale                 | Protocol   | Priority |
| --------------------- | ---------- | -------- |
| Acaia Lunar           | Acaia BLE  | High     |
| Acaia Pearl           | Acaia BLE  | High     |
| Decent Scale          | Decent BLE | Medium   |
| Timemore Black Mirror | Generic    | Low      |

### Features

| Feature          | Description               |
| ---------------- | ------------------------- |
| Auto-connect     | Reconnect to paired scale |
| Weight streaming | Real-time weight display  |
| Tare             | Remote tare command       |
| Auto-stop        | Stop at target weight     |

### Tasks

```
[x] BLE-1: NimBLE stack initialization
[x] BLE-2: Scale scanning and pairing
[x] BLE-3: Acaia protocol implementation
[x] BLE-4: Weight notification handling
[x] BLE-5: Auto-stop signal to Pico
[x] BLE-6: Scale config persistence
[x] BLE-7: Felicita/Decent/Timemore support
[x] BLE-8: Flow rate calculation
```

---

## Memory Budget

| Component           | RAM     | Notes                 |
| ------------------- | ------- | --------------------- |
| LVGL Core           | ~64 KB  | Graphics library      |
| LVGL Display Buffer | ~38 KB  | 480×40 lines          |
| WiFi Stack          | ~40 KB  | ESP-IDF managed       |
| Web Server          | ~20 KB  | AsyncWebServer        |
| MQTT Client         | ~5 KB   | PubSubClient          |
| BLE Stack           | ~50 KB  | NimBLE                |
| JSON Buffers        | ~8 KB   | ArduinoJson           |
| **Total**           | ~225 KB | From 328 KB available |

---

## File Structure

```
src/esp32/
├── include/
│   ├── config.h                    # Configuration constants
│   ├── mqtt_client.h               # MQTT client
│   ├── pico_uart.h                 # UART bridge to Pico
│   ├── web_server.h                # Web server
│   ├── wifi_manager.h              # WiFi management
│   ├── cloud_connection.h          # Cloud WebSocket client
│   ├── pairing_manager.h           # QR code device pairing
│   ├── brew_by_weight.h            # Brew-by-weight logic
│   ├── display/
│   │   ├── display.h
│   │   ├── display_config.h
│   │   ├── encoder.h
│   │   ├── theme.h
│   │   └── lv_fs_littlefs.h
│   ├── notifications/
│   │   ├── notification_manager.h
│   │   ├── notification_types.h
│   │   └── cloud_notifier.h
│   ├── power_meter/
│   │   ├── power_meter.h
│   │   ├── power_meter_manager.h
│   │   └── mqtt_power_meter.h
│   ├── scale/
│   │   ├── scale_interface.h
│   │   └── scale_manager.h
│   ├── state/
│   │   ├── state_manager.h
│   │   └── state_types.h
│   ├── statistics/
│   │   └── statistics_manager.h
│   └── ui/
│       ├── ui.h
│       ├── screen_alarm.h
│       ├── screen_bbw.h
│       ├── screen_brewing.h
│       ├── screen_cloud.h
│       ├── screen_complete.h
│       ├── screen_home.h
│       ├── screen_idle.h
│       ├── screen_ota.h
│       ├── screen_scale.h
│       ├── screen_settings.h
│       ├── screen_setup.h
│       ├── screen_splash.h
│       └── screen_temp.h
├── src/
│   ├── main.cpp
│   ├── mqtt_client.cpp
│   ├── pico_uart.cpp
│   ├── web_server.cpp
│   ├── web_server_broadcast.cpp
│   ├── web_server_ota.cpp
│   ├── web_server_websocket.cpp
│   ├── wifi_manager.cpp
│   ├── cloud_connection.cpp
│   ├── pairing_manager.cpp
│   ├── brew_by_weight.cpp
│   ├── display/
│   │   ├── display.cpp
│   │   ├── encoder.cpp
│   │   ├── lv_fs_littlefs.cpp
│   │   └── theme.cpp
│   ├── notifications/
│   │   ├── notification_manager.cpp
│   │   └── cloud_notifier.cpp
│   ├── power_meter/
│   │   ├── power_meter_manager.cpp
│   │   └── mqtt_power_meter.cpp
│   ├── scale/
│   │   ├── scale_factory.cpp
│   │   └── scale_manager.cpp
│   ├── state/
│   │   ├── state_manager.cpp
│   │   └── state_types.cpp
│   ├── statistics/
│   │   └── statistics_manager.cpp
│   └── ui/
│       ├── ui.cpp
│       ├── screen_alarm.cpp
│       ├── screen_bbw.cpp
│       ├── screen_brewing.cpp
│       ├── screen_cloud.cpp
│       ├── screen_complete.cpp
│       ├── screen_home.cpp
│       ├── screen_idle.cpp
│       ├── screen_ota.cpp
│       ├── screen_scale.cpp
│       ├── screen_settings.cpp
│       ├── screen_setup.cpp
│       ├── screen_splash.cpp
│       └── screen_temp.cpp
├── data/                           # Web UI (LittleFS)
└── platformio.ini
```

---

## Dependencies

| Library             | Version | Purpose        |
| ------------------- | ------- | -------------- |
| ESP Async WebServer | latest  | HTTP server    |
| AsyncTCP            | 1.1.1   | TCP for ESP32  |
| ArduinoJson         | latest  | JSON parsing   |
| lvgl                | 8.3.x   | Graphics       |
| LovyanGFX           | 1.1.x   | Display driver |
| PubSubClient        | 2.8     | MQTT           |
| NimBLE-Arduino      | 1.4.x   | BLE            |

---

## Related Documentation

- [MQTT Integration](integrations/MQTT.md)
- [Web API Reference](integrations/Web_API.md)
- [Communication Protocol](../shared/Communication_Protocol.md)
