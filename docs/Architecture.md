# BrewOS System Architecture

## High-Level Overview

![BrewOS System Architecture](images/architecture.png)

The diagram shows the complete BrewOS ecosystem:

- **Cloud Layer** (`cloud.brewos.io`) - Google OAuth, Node.js WebSocket relay, SQLite database
- **Home Network** - ESP32-S3 connectivity hub with React Web UI, cloud client, MQTT, and BLE
- **Real-time Controller** - Pico RP2350 handling PID, boiler, pump, and valve control
- **Machine Hardware** - SSRs, pumps, valves, and temperature sensors

## Communication Paths

### Local Access (Home Network)
```
Phone/Laptop ──HTTP/WS──► ESP32 (brewos.local)
```
- Direct connection to ESP32 web server
- Works without internet
- Full control + real-time updates

### Remote Access (Cloud)
```
Phone/Laptop ──WSS──► Cloud Server ──WSS──► ESP32
```
- WebSocket relay through cloud.brewos.io
- Requires ESP32 connected to internet
- Same UI and functionality as local

### Home Automation (MQTT)
```
Home Assistant ──MQTT──► ESP32 ──► Pico
```
- Auto-discovery for Home Assistant
- Control via MQTT commands
- Status updates published

### BLE Scale Integration
```
Bluetooth Scale ──BLE──► ESP32 ──► Pico
```
- Real-time weight data
- Brew-by-weight automation
- Stop shot at target weight

## Data Flow

### Shot Execution
```
User Request → ESP32 → Pico → Machine Hardware
     │           │       │
     │           │       └── Real-time control (10ms loop)
     │           │
     │           └── State management, logging, cloud sync
     │
     └── Web UI / Cloud / MQTT / Display
```

### State Management

![State Management](images/esp_state_management.png)


## User Onboarding Flow

![Onboarding Flow](images/onboarding_flow.png)

## Component Summary

| Component | Technology | Responsibility |
|-----------|------------|----------------|
| **Pico** | C, RP2350 | Real-time machine control, safety |
| **ESP32** | C++, ESP-IDF | Connectivity, UI, state management |
| **Web UI** | React, TypeScript | Dashboard, settings, control |
| **Cloud** | Node.js, Express | WebSocket relay, auth, device registry |
| **Display** | LVGL | Touchscreen interface on machine |
| **Storage** | NVS + LittleFS | Settings, stats, shot history |
| **Database** | SQLite | Cloud user/device management |

## Security Model

| Layer | Mechanism |
|-------|-----------|
| **Cloud Access** | Google OAuth → ID Token → Server Verification |
| **Device Pairing** | QR Code (HMAC Token) → User Claim → Device Ownership |
| **Local Access** | Same network only → No auth required (trusted network) |
| **MQTT** | Username/Password + TLS → Broker → Home Assistant |

## Related Documentation

- [Pico Architecture](pico/Architecture.md) - Real-time control details
- [ESP32 State Management](esp32/State_Management.md) - Data persistence
- [Cloud Service](cloud/README.md) - WebSocket relay details
- [WebSocket Protocol](web/WebSocket_Protocol.md) - Message formats
- [Communication Protocol](shared/Communication_Protocol.md) - ESP32 ↔ Pico
