# ESP32 Firmware Documentation

The ESP32-S3 handles connectivity, user interface, and advanced features for the BrewOS coffee machine controller.

## Quick Links

| Document                                      | Description                    |
| --------------------------------------------- | ------------------------------ |
| [Implementation Plan](Implementation_Plan.md) | Development roadmap and status |
| [UI Design](UI_Design.md)                     | Display screens and navigation |
| [UI Simulator](Simulator.md)                  | Desktop UI preview tool        |
| [OTA Updates](OTA_Updates.md)                 | Firmware update mechanism      |

### Features

| Document                                            | Description                            |
| --------------------------------------------------- | -------------------------------------- |
| [Schedules & Auto Power-Off](features/Schedules.md) | Time-based automation                  |
| [Eco Mode](features/Eco_Mode.md)                    | Power-saving idle mode                 |
| [Statistics](features/Statistics.md)                | Usage stats, brew history, power graph |
| [Brew-by-Weight](integrations/Brew_By_Weight.md)    | Auto-stop at target weight             |
| [BLE Scales](integrations/BLE_Scales.md)            | Bluetooth scale integration            |

### Integrations

| Document                                       | Description                   |
| ---------------------------------------------- | ----------------------------- |
| [MQTT Integration](integrations/MQTT.md)       | MQTT setup and Home Assistant |
| [Web API Reference](integrations/Web_API.md)   | HTTP endpoints and WebSocket  |
| [Notifications](integrations/Notifications.md) | Push reminders and alerts     |

### Power Monitoring

| Document                                                              | Description                              |
| --------------------------------------------------------------------- | ---------------------------------------- |
| [MQTT Power Metering](https://github.com/brewos-io/app/blob/main/docs/Power_Metering.md#mqtt-smart-plug-setup) | Smart plug integration (Shelly, Tasmota) |

> **Note:** Hardware power meters (PZEM, JSY, Eastron) are handled by Pico. See [Pico Power Metering](../pico/Power_Metering.md).

## Hardware

- **MCU:** ESP32-S3
- **Display:** 2.1" Round IPS (480×480)
- **Input:** Rotary Encoder + Push Button
- **Memory:** 8 MB PSRAM, 16 MB Flash

## Features

### Implemented ✅

- WiFi (AP setup + STA mode)
- Web dashboard with real-time updates
- UART bridge to Pico
- OTA Pico firmware updates
- MQTT with Home Assistant discovery
- RESTful API
- BLE scale integration (Acaia, Felicita, Decent, Timemore)
- Brew-by-weight auto-stop
- Temperature control via web/MQTT
- **Schedules** - Turn machine on/off at specific times
- **Auto Power-Off** - Turn off after idle period
- **Eco Mode** - Reduce temperature when idle, wake on activity
- **Time/NTP Settings** - Timezone and NTP server configuration
- **Statistics** - Comprehensive brew tracking with power graphs and history

### Implemented ✅ (continued)

- **Notifications** - Push reminders and alerts via WebSocket, MQTT, and Cloud
- **Cloud Integration** - Remote access via cloud.brewos.io WebSocket relay
- **QR Code Device Pairing** - Scan to link devices to user accounts

## Building

```bash
cd src/esp32
pio run              # Build for ESP32-S3
pio run -t upload    # Flash to device
pio run -t uploadfs  # Upload web UI files
pio run -e simulator # Build UI simulator
```

### UI Simulator

Preview theme colors and UI elements on your desktop:

```bash
./src/scripts/run_simulator.sh
```

See [Simulator.md](Simulator.md) for details.

## Configuration

1. Power on device - creates `BrewOS-Setup` WiFi access point
2. Connect to WiFi network:
   - **SSID:** `BrewOS-Setup`
   - **Password:** `brewoscoffee`
   - The password is displayed on the ESP32 screen
3. Open `http://192.168.4.1` in your browser
4. Configure WiFi and MQTT
5. Device restarts and connects to network

## Source Code Structure

```
src/esp32/
├── include/
│   ├── config.h                    # Configuration constants
│   ├── mqtt_client.h               # MQTT client interface
│   ├── pico_uart.h                 # UART bridge to Pico
│   ├── web_server.h                # Web server interface
│   ├── wifi_manager.h              # WiFi management
│   ├── cloud_connection.h          # Cloud WebSocket client
│   ├── pairing_manager.h           # QR code device pairing
│   ├── brew_by_weight.h            # Brew-by-weight logic
│   ├── display/                    # Display drivers
│   ├── notifications/              # Notification system
│   │   ├── notification_manager.h
│   │   ├── notification_types.h
│   │   └── cloud_notifier.h
│   ├── power_meter/                # Power monitoring
│   ├── scale/                      # BLE scale integration
│   ├── state/                      # State management
│   ├── statistics/                 # Usage statistics
│   └── ui/                         # LVGL screen definitions
├── src/
│   ├── main.cpp                    # Entry point
│   ├── mqtt_client.cpp
│   ├── pico_uart.cpp
│   ├── web_server.cpp              # HTTP/WebSocket server
│   ├── wifi_manager.cpp
│   ├── cloud_connection.cpp        # Cloud WebSocket client
│   ├── pairing_manager.cpp         # QR code pairing
│   ├── brew_by_weight.cpp
│   ├── display/                    # Display implementation
│   ├── notifications/              # Notification system
│   ├── power_meter/                # Power meter manager
│   ├── scale/                      # Scale manager
│   ├── state/                      # State manager
│   ├── statistics/                 # Statistics manager
│   └── ui/                         # UI screens
├── data/                           # Web UI (LittleFS)
└── platformio.ini

docs/esp32/
├── README.md              # This file
├── Implementation_Plan.md # Development status
├── UI_Design.md           # Display UI specification
├── Simulator.md           # Desktop UI simulator
├── OTA_Updates.md         # Firmware update mechanism
├── State_Management.md    # State architecture
├── features/
│   ├── Schedules.md       # Schedules & auto power-off
│   ├── Eco_Mode.md        # Eco mode (power saving)
│   └── Statistics.md      # Usage stats, brew history, power
└── integrations/
    ├── MQTT.md            # MQTT documentation
    ├── Web_API.md         # API reference
    ├── BLE_Scales.md      # Bluetooth scales
    ├── Brew_By_Weight.md  # Auto-stop feature
    └── Notifications.md   # Push reminders & alerts
```
