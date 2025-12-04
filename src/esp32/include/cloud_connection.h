#ifndef CLOUD_CONNECTION_H
#define CLOUD_CONNECTION_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <functional>

/**
 * CloudConnection
 * 
 * WebSocket client that maintains a persistent connection to the BrewOS cloud.
 * Enables remote access: cloud users receive real-time state updates and can
 * send commands to the device.
 * 
 * Protocol:
 * - Connect to: wss://cloud.server/ws/device?id=DEVICE_ID&key=DEVICE_KEY
 * - Messages are JSON with { type: "...", ... } format
 * - Receives commands from cloud users (forwarded to command handler)
 * - Sends state updates to cloud (broadcast to all connected cloud users)
 */
class CloudConnection {
public:
    // Command handler callback - receives commands from cloud users
    using CommandCallback = std::function<void(const String& type, JsonDocument& doc)>;
    
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
     * Send JSON message to cloud (broadcast to all connected cloud users)
     */
    void send(const String& json);
    
    /**
     * Send typed message to cloud
     */
    void send(const JsonDocument& doc);
    
    /**
     * Set callback for receiving commands from cloud users
     */
    void onCommand(CommandCallback callback);
    
    /**
     * Check if connected to cloud
     */
    bool isConnected() const;
    
    /**
     * Get connection status string
     */
    String getStatus() const;
    
    /**
     * Enable/disable connection (without clearing config)
     */
    void setEnabled(bool enabled);
    
    /**
     * Check if cloud is enabled
     */
    bool isEnabled() const;

private:
    WebSocketsClient _ws;
    String _serverUrl;
    String _deviceId;
    String _deviceKey;
    bool _enabled = false;
    bool _connected = false;
    bool _connecting = false;
    unsigned long _lastConnectAttempt = 0;
    unsigned long _reconnectDelay = 1000;  // Start with 1 second
    static const unsigned long MAX_RECONNECT_DELAY = 10000;  // Max 10 seconds
    
    CommandCallback _onCommand = nullptr;
    
    // Parse URL into host, port, path
    bool parseUrl(const String& url, String& host, uint16_t& port, String& path, bool& useSSL);
    
    // WebSocket event handler
    void handleEvent(WStype_t type, uint8_t* payload, size_t length);
    
    // Process incoming message
    void handleMessage(uint8_t* payload, size_t length);
    
    // Attempt to connect
    void connect();
};

#endif // CLOUD_CONNECTION_H

