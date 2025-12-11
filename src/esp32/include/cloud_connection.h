#ifndef CLOUD_CONNECTION_H
#define CLOUD_CONNECTION_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

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
    // Simple function pointer to avoid std::function PSRAM allocation issues
    typedef void (*CommandCallback)(const String& type, JsonDocument& doc);
    
    // Registration callback - called before first connect to register device with cloud
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
     * Send JSON message to cloud (broadcast to all connected cloud users)
     */
    void send(const String& json);
    void send(const char* json);  // Overload to avoid String allocation
    
    /**
     * Send typed message to cloud
     */
    void send(const JsonDocument& doc);
    
    /**
     * Set callback for receiving commands from cloud users
     */
    void onCommand(CommandCallback callback);
    
    /**
     * Set callback for registering device with cloud before connecting
     * Called once before first connection attempt (when WiFi is available)
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
    unsigned long _lastConnectedTime = 0;       // When we last successfully connected
    unsigned long _reconnectDelay = 5000;       // Start with 5 seconds
    int _quickDisconnectCount = 0;              // Count of quick disconnects (server rejections)
    bool _inLongBackoff = false;                // True if we're in a long backoff period
    static const unsigned long MAX_RECONNECT_DELAY = 60000;  // Max 60 seconds
    
    CommandCallback _onCommand = nullptr;
    RegisterCallback _onRegister = nullptr;
    bool _registered = false;  // True after successful registration
    
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

