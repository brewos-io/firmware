#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "ui/ui.h"

// Forward declarations
class PicoUART;
class MQTTClient;
class PairingManager;

class WebServer {
public:
    WebServer(WiFiManager& wifiManager, PicoUART& picoUart, MQTTClient& mqttClient, PairingManager* pairingManager = nullptr);
    
    // Initialize
    void begin();
    
    // Update - call in loop
    void loop();
    
    // Send data to all WebSocket clients - Unified Status Broadcast
    void broadcastFullStatus(const struct ui_state_t& machineState);  // Comprehensive status (periodic)
    void broadcastDeviceInfo();   // Device info (on connect only)
    void broadcastLog(const String& message, const String& level = "info");  // Log messages
    void broadcastEvent(const String& event, const JsonDocument* data = nullptr);  // Events (shot_start, shot_end, etc.)
    
    // Legacy/debug - raw pico message forwarding
    void broadcastPicoMessage(uint8_t type, const uint8_t* payload, size_t len);
    
    // Get client count
    size_t getClientCount();

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    WiFiManager& _wifiManager;
    PicoUART& _picoUart;
    MQTTClient& _mqttClient;
    PairingManager* _pairingManager;
    
    // Request handlers
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest* request);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetWiFiNetworks(AsyncWebServerRequest* request);
    void handleSetWiFi(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handleCommand(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleGetMQTTConfig(AsyncWebServerRequest* request);
    void handleSetMQTTConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleTestMQTT(AsyncWebServerRequest* request);
    void handleOTAUpload(AsyncWebServerRequest* request, const String& filename,
                         size_t index, uint8_t* data, size_t len, bool final);
    void handleStartOTA(AsyncWebServerRequest* request);
    
    // WebSocket handlers
    void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                   AwsEventType type, void* arg, uint8_t* data, size_t len);
    void handleWsMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
    
    // Helpers
    String getContentType(const String& filename);
    bool streamFirmwareToPico(File& firmwareFile, size_t firmwareSize);
};

#endif // WEB_SERVER_H

