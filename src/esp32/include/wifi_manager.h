#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>

// WiFi modes
enum class WiFiManagerMode {
    DISCONNECTED,
    AP_MODE,        // Access Point (setup mode)
    STA_MODE,       // Station (connected to router)
    STA_CONNECTING  // Trying to connect
};

// Static IP configuration
struct StaticIPConfig {
    bool enabled;
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
};

// WiFi status for web UI
struct WiFiStatus {
    WiFiManagerMode mode;
    String ssid;
    String ip;
    int8_t rssi;
    bool configured;
    // Static IP info
    bool staticIp;
    String gateway;
    String subnet;
    String dns1;
    String dns2;
};

// Time/NTP status
struct TimeStatus {
    bool ntpSynced;
    String currentTime;     // Formatted local time
    String timezone;        // Current timezone description
    int32_t utcOffset;      // Current UTC offset in seconds
};

// Simple function pointer types to avoid std::function PSRAM allocation issues
typedef void (*WiFiEventCallback)();

class WiFiManager {
public:
    WiFiManager();
    
    // Initialize - tries STA if configured, falls back to AP
    void begin();
    
    // Update - call in loop
    void loop();
    
    // Configuration
    bool hasStoredCredentials();
    bool setCredentials(const String& ssid, const String& password);
    void clearCredentials();
    
    // Static IP configuration
    void setStaticIP(bool enabled, const String& ip = "", const String& gateway = "", 
                     const String& subnet = "255.255.255.0", const String& dns1 = "", const String& dns2 = "");
    StaticIPConfig getStaticIPConfig() const { return _staticIP; }
    
    // Connect to stored WiFi
    bool connectToWiFi();
    
    // Start AP mode (setup)
    void startAP();
    
    // Get current status
    WiFiStatus getStatus();
    WiFiManagerMode getMode() { return _mode; }
    bool isAPMode() { return _mode == WiFiManagerMode::AP_MODE; }
    bool isConnected();
    String getIP();
    
    // Get stored credentials (raw pointers to internal buffers)
    const char* getStoredSSID() const { return _storedSSID; }
    
    // NTP/Time configuration
    void configureNTP(const char* server, int16_t utcOffsetMinutes, bool dstEnabled, int16_t dstOffsetMinutes);
    void syncNTP();
    bool isTimeSynced();
    TimeStatus getTimeStatus();
    time_t getLocalTime();
    String getFormattedTime(const char* format = "%Y-%m-%d %H:%M:%S");
    
    // Events - use simple function pointers to avoid std::function PSRAM issues
    void onConnected(WiFiEventCallback callback) { _onConnected = callback; }
    void onDisconnected(WiFiEventCallback callback) { _onDisconnected = callback; }
    void onAPStarted(WiFiEventCallback callback) { _onAPStarted = callback; }

private:
    WiFiManagerMode _mode;
    Preferences _prefs;
    
    // Fixed-size buffers to avoid String/PSRAM allocations
    char _storedSSID[64];
    char _storedPassword[128];
    
    // Static IP configuration
    StaticIPConfig _staticIP = {false, IPAddress(), IPAddress(), IPAddress(255,255,255,0), IPAddress(), IPAddress()};
    
    unsigned long _lastConnectAttempt;
    unsigned long _connectStartTime;
    
    // NTP settings
    char _ntpServer[64] = "pool.ntp.org";
    int32_t _utcOffsetSec = 0;
    int32_t _dstOffsetSec = 0;
    bool _ntpConfigured = false;
    
    // Simple function pointers - no dynamic allocation
    WiFiEventCallback _onConnected = nullptr;
    WiFiEventCallback _onDisconnected = nullptr;
    WiFiEventCallback _onAPStarted = nullptr;
    
    void loadCredentials();
    void saveCredentials(const String& ssid, const String& password);
    void loadStaticIPConfig();
    void saveStaticIPConfig();
};

#endif // WIFI_MANAGER_H
