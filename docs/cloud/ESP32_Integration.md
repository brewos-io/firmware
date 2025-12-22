# ESP32 Cloud Integration

This guide explains how to connect your BrewOS ESP32 device to the cloud service for remote access.

## Overview

The ESP32 maintains a persistent WebSocket connection to the cloud service, enabling:

- Remote monitoring from anywhere
- Remote control via web/mobile apps
- Multi-device management

## Connection Flow

```
┌─────────┐                    ┌─────────────┐                    ┌────────┐
│  ESP32  │                    │    Cloud    │                    │  App   │
└────┬────┘                    └──────┬──────┘                    └───┬────┘
     │                                │                               │
     │  1. Connect (id + key)         │                               │
     │ ──────────────────────────────►│                               │
     │                                │                               │
     │  2. { type: "connected" }      │                               │
     │ ◄──────────────────────────────│                               │
     │                                │                               │
     │                                │  3. Connect (token + device)  │
     │                                │◄──────────────────────────────│
     │                                │                               │
     │                                │  4. { type: "connected",      │
     │                                │       deviceOnline: true }    │
     │                                │──────────────────────────────►│
     │                                │                               │
     │  5. Status updates             │                               │
     │ ──────────────────────────────►│──────────────────────────────►│
     │                                │                               │
     │                                │  6. Commands                  │
     │ ◄──────────────────────────────│◄──────────────────────────────│
     │                                │                               │
```

## ESP32 Implementation

The ESP32 firmware includes a `CloudConnection` class that handles the cloud WebSocket connection.

### CloudConnection Class

```cpp
// cloud_connection.h
#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class CloudConnection {
public:
    // Command handler callback
    typedef void (*CommandCallback)(const String& type, JsonDocument& doc);
    
    // Registration callback - called before first connect
    typedef bool (*RegisterCallback)();
    
    CloudConnection();
    
    /**
     * Initialize cloud connection
     * @param serverUrl Cloud server URL (e.g., "https://cloud.brewos.io")
     * @param deviceId Device identifier (e.g., "BRW-XXXXXXXX")
     * @param deviceKey Secret key for authentication
     */
    void begin(const String& serverUrl, const String& deviceId, const String& deviceKey);
    
    /**
     * Disconnect and disable cloud connection
     */
    void end();
    
    /**
     * Call in loop() - handles reconnection and message processing
     */
    void loop();
    
    /**
     * Send JSON message to cloud
     */
    void send(const String& json);
    void send(const char* json);
    void send(const JsonDocument& doc);
    
    /**
     * Set callback for receiving commands from cloud users
     */
    void onCommand(CommandCallback callback);
    
    /**
     * Set callback for registering device with cloud before connecting
     */
    void onRegister(RegisterCallback callback);
    
    /**
     * Check if connected to cloud
     */
    bool isConnected() const;
    
    /**
     * Get connection status string
     */
    String getStatus() const;
    
    /**
     * Enable/disable connection
     */
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    /**
     * Pause connection to free up resources
     */
    void pause();
};
```

### Usage Example

The CloudConnection is initialized in `main.cpp`:

```cpp
#include "cloud_connection.h"
#include "pairing_manager.h"

CloudConnection cloudConnection;
PairingManager pairingManager;

void setup() {
    // ... WiFi setup ...
    
    // Initialize pairing manager (generates device key on first boot)
    pairingManager.begin("https://cloud.brewos.io");
    
    // Set up cloud connection with device credentials
    cloudConnection.begin(
        "https://cloud.brewos.io",
        pairingManager.getDeviceId(),  // e.g., "BRW-12345678"
        pairingManager.getDeviceKey()  // Auto-generated secret key
    );
    
    // Set up command handler
    cloudConnection.onCommand([](const String& type, JsonDocument& doc) {
        // Commands from cloud users are forwarded to the same
        // handler as local WebSocket commands
        handleCommand(type, doc);
    });
    
    // Set up registration callback (called before first connection)
    cloudConnection.onRegister([]() {
        return pairingManager.registerTokenWithCloud();
    });
}

void loop() {
    cloudConnection.loop();
    
    // State updates are automatically broadcast to both
    // local WebSocket clients and cloud connection
}
```

### Key Differences from Example Code

The actual implementation:

1. **Uses simple function pointers** instead of `std::function` to avoid PSRAM allocation issues
2. **Includes registration callback** for QR code pairing flow
3. **Has pause/resume capability** for resource management during OTA updates
4. **Integrates with PairingManager** which handles device key generation and QR code pairing

## Configuration

### Device Registration

Each device is identified by:

1. **Device ID** - Format: `BRW-XXXXXXXX` (derived from ESP32 chip ID)
2. **Device Key** - Cryptographically random 32-byte key, base64url encoded

The `PairingManager` class handles all of this automatically:

```cpp
// pairing_manager.h (excerpt)
class PairingManager {
public:
    void begin(const String& cloudUrl = "");
    
    String getDeviceId() const;     // e.g., "BRW-12345678"
    String getDeviceKey() const;    // Auto-generated secret
    String getPairingUrl() const;   // URL for QR code
    String generateToken();         // Generate new pairing token
    bool registerTokenWithCloud();  // Register with cloud service
};
```

The device key is:
- Generated on first boot using `esp_fill_random()`
- Stored in NVS under `brewos_sec/devKey`
- Sent to cloud during QR code pairing registration
- Used for every WebSocket connection authentication

### Cloud URL Configuration

The cloud URL is stored in NVS and can be configured via the web UI:

```cpp
// Default: "https://cloud.brewos.io"
String cloudUrl = preferences.getString("cloud_url", "https://cloud.brewos.io");
```

## Security

### Device Key Authentication

Every ESP32 device has a unique device key used for authentication:

1. **First Boot**: ESP32 generates a cryptographically random device key (32 bytes, base64url encoded)
2. **NVS Storage**: Key is persisted in secure NVS partition under `brewos_sec/devKey`
3. **Registration**: During pairing (QR code scan), ESP32 sends key to cloud via `/api/devices/register-claim`
4. **Connection**: Every WebSocket connection includes the key as a query parameter
5. **Validation**: Cloud verifies key hash before allowing connection

```cpp
// Key generation happens automatically in PairingManager
// Connection URL includes key:
// wss://cloud.brewos.io/ws/device?id=BRW-12345678&key=<device_key>
```

### TLS/SSL

Always use WSS (WebSocket Secure) in production:

- ESP32 supports TLS 1.2
- Verify server certificate if possible

### Device Key Rotation

Implement key rotation:

1. Cloud generates new key
2. Sends `key_rotate` command with new key
3. ESP32 saves new key, reconnects

### Secure Storage

Store device key in encrypted NVS partition.

## Reconnection

Handle disconnections gracefully:

```cpp
void CloudClient::loop() {
    _ws.loop();

    // Manual reconnection with backoff
    if (!_connected && millis() - _lastReconnect > _reconnectDelay) {
        _lastReconnect = millis();
        _reconnectDelay = min(_reconnectDelay * 2, 60000UL);  // Max 1 minute
        Serial.println("[Cloud] Reconnecting...");
    }

    if (_connected) {
        _reconnectDelay = 5000;  // Reset on successful connection
    }
}
```

## Status Messages

Send regular status updates:

```cpp
void sendFullStatus() {
    StaticJsonDocument<512> doc;
    doc["type"] = "pico_status";
    doc["version"] = FIRMWARE_VERSION;
    doc["uptime"] = millis();
    doc["state"] = getMachineState();
    doc["brewTemp"] = brewBoiler.getTemperature();
    doc["brewSetpoint"] = brewBoiler.getSetpoint();
    doc["steamTemp"] = steamBoiler.getTemperature();
    doc["steamSetpoint"] = steamBoiler.getSetpoint();
    doc["pressure"] = getPressure();
    doc["power"] = getPower();
    doc["isHeating"] = isHeating();
    doc["isBrewing"] = isBrewing();

    String json;
    serializeJson(doc, json);
    cloudClient.sendStatus(json.c_str());
}
```

## Bandwidth Optimization

Minimize data usage:

1. Send status updates every 2-5 seconds (not faster)
2. Use short field names in JSON
3. Only send changed values when possible
4. Batch multiple updates into single message

## Offline Operation

The ESP32 should work fully offline:

- Cloud connection is optional
- Local WebSocket still works
- Queue commands when offline (optional)

```cpp
bool hasCloudConnection() {
    return cloudClient.isConnected();
}

bool hasLocalConnection() {
    return webSocket.connectedClients() > 0;
}

bool hasAnyConnection() {
    return hasCloudConnection() || hasLocalConnection();
}
```
