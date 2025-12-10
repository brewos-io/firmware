#include "web_server.h"
#include "config.h"
#include "pico_uart.h"
#include "mqtt_client.h"
#include "brew_by_weight.h"
#include "scale/scale_manager.h"
#include "pairing_manager.h"
#include "cloud_connection.h"
#include "state/state_manager.h"
#include "statistics/statistics_manager.h"
#include "power_meter/power_meter_manager.h"
#include <LittleFS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_heap_caps.h>
#include <pgmspace.h>
#include <stdarg.h>
#include <stdio.h>

// Deferred WiFi connection state (allows HTTP response to be sent before disconnecting AP)
static bool _pendingWiFiConnect = false;
static unsigned long _wifiConnectRequestTime = 0;

// Track when WiFi is ready to serve requests (prevents PSRAM crashes from early HTTP requests)
static unsigned long _wifiReadyTime = 0;
static const unsigned long WIFI_READY_DELAY_MS = 5000;  // 5 seconds after WiFi connects

// Static WebServer pointer for WebSocket callback
static WebServer* _wsInstance = nullptr;

// Note: All JsonDocument instances should use StaticJsonDocument with stack allocation
// to avoid PSRAM crashes. Use the pragma pattern from handleGetWiFiNetworks.

WebServer::WebServer(WiFiManager& wifiManager, PicoUART& picoUart, MQTTClient& mqttClient, PairingManager* pairingManager)
    : _server(WEB_SERVER_PORT)
    , _ws("/ws")  // WebSocket on same port 80, endpoint /ws
    , _wifiManager(wifiManager)
    , _picoUart(picoUart)
    , _mqttClient(mqttClient)
    , _pairingManager(pairingManager) {
}

void WebServer::begin() {
    LOG_I("Starting web server...");
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        LOG_E("Failed to mount LittleFS");
    } else {
        LOG_I("LittleFS mounted");
    }
    
    // Setup routes
    setupRoutes();
    
    // Setup WebSocket handler
    _wsInstance = this;
    _ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (_wsInstance) {
            _wsInstance->handleWsEvent(server, client, type, arg, data, len);
        }
    });
    _server.addHandler(&_ws);
    
    // Start HTTP server
    _server.begin();
    LOG_I("HTTP server started on port %d", WEB_SERVER_PORT);
    LOG_I("WebSocket available at ws://brewos.local/ws");
}

void WebServer::setCloudConnection(CloudConnection* cloudConnection) {
    _cloudConnection = cloudConnection;
}

void WebServer::setWiFiConnected() {
    _wifiReadyTime = millis();
    LOG_I("WiFi connected - requests will be served after %lu ms delay", WIFI_READY_DELAY_MS);
}

bool WebServer::isWiFiReady() {
    if (_wifiReadyTime == 0) {
        return false;  // WiFi not connected yet
    }
    return (millis() - _wifiReadyTime) >= WIFI_READY_DELAY_MS;
}

// The React app is served from LittleFS via serveStatic()
// Users can access it at http://brewos.local after WiFi connects

void WebServer::loop() {
    // AsyncWebSocket is event-driven, no loop() needed
    // Periodically clean up disconnected clients
    static unsigned long lastCleanup = 0;
    if (millis() - lastCleanup > 1000) {
        _ws.cleanupClients();
        lastCleanup = millis();
    }
    
    // Handle deferred WiFi connection
    // Wait 500ms after request to ensure HTTP response is fully sent
    if (_pendingWiFiConnect && _wifiConnectRequestTime == 0) {
        _wifiConnectRequestTime = millis();
    }
    
    if (_pendingWiFiConnect && _wifiConnectRequestTime > 0 && 
        millis() - _wifiConnectRequestTime > 500) {
        _pendingWiFiConnect = false;
        _wifiConnectRequestTime = 0;
        LOG_I("Starting WiFi connection (deferred)");
        _wifiManager.connectToWiFi();
    }
}

void WebServer::setupRoutes() {
    // Simple test endpoint - no LittleFS needed
    _server.on("/test", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "BrewOS Web Server OK");
    });
    
    // WiFi Setup page - inline HTML (no file operations, no PSRAM issues)
    // This follows IoT best practices: minimal setup page served directly
    _server.on("/setup", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Inline HTML WiFi setup page - completely self-contained
        // No file operations, no PSRAM usage, works reliably
        // Using PROGMEM to store in flash (not RAM/PSRAM)
        const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>BrewOS WiFi Setup</title>
    <style>
        *{box-sizing:border-box;margin:0;padding:0}
        body{font-family:'Inter',-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:linear-gradient(145deg,#1a1412 0%,#2d1f18 50%,#1a1412 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
        .card{background:linear-gradient(180deg,#1e1714 0%,#171210 100%);border-radius:24px;box-shadow:0 25px 80px rgba(0,0,0,0.5),0 0 0 1px rgba(186,132,86,0.1);max-width:420px;width:100%;padding:40px 32px;position:relative;overflow:hidden}
        .card::before{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:linear-gradient(90deg,#ba8456,#c38f5f,#a06b3d)}
        .logo{width:80px;height:80px;margin:0 auto 24px;display:block;filter:drop-shadow(0 4px 12px rgba(186,132,86,0.3))}
        h1{color:#f5f0eb;text-align:center;margin-bottom:8px;font-size:26px;font-weight:600;letter-spacing:-0.5px}
        .subtitle{color:#9a8578;text-align:center;margin-bottom:32px;font-size:14px}
        .form-group{margin-bottom:20px}
        label{display:block;color:#c4b5a9;font-weight:500;margin-bottom:10px;font-size:13px;text-transform:uppercase;letter-spacing:0.5px}
        input{width:100%;padding:14px 16px;background:#0d0a09;border:1px solid #3d2e24;border-radius:12px;font-size:15px;color:#f5f0eb;transition:all 0.2s}
        input::placeholder{color:#5c4d42}
        input:focus{outline:none;border-color:#ba8456;box-shadow:0 0 0 3px rgba(186,132,86,0.15)}
        .btn{width:100%;padding:16px;background:linear-gradient(135deg,#ba8456 0%,#a06b3d 100%);color:#fff;border:none;border-radius:12px;font-size:15px;font-weight:600;cursor:pointer;transition:all 0.2s;text-transform:uppercase;letter-spacing:0.5px}
        .btn:hover{transform:translateY(-1px);box-shadow:0 8px 24px rgba(186,132,86,0.3)}
        .btn:active{transform:translateY(0)}
        .btn:disabled{opacity:0.4;cursor:not-allowed;transform:none}
        .btn-secondary{background:#2d241e;color:#c4b5a9;margin-top:12px}
        .btn-secondary:hover{background:#3d2e24}
        .status{margin-top:20px;padding:14px 16px;border-radius:12px;font-size:14px;display:none;text-align:center}
        .status.success{background:rgba(34,197,94,0.1);color:#4ade80;border:1px solid rgba(34,197,94,0.2)}
        .status.error{background:rgba(239,68,68,0.1);color:#f87171;border:1px solid rgba(239,68,68,0.2)}
        .status.info{background:rgba(186,132,86,0.1);color:#d5a071;border:1px solid rgba(186,132,86,0.2)}
        .network-list{max-height:280px;overflow-y:auto;background:#0d0a09;border:1px solid #3d2e24;border-radius:12px;margin-bottom:16px}
        .network-list::-webkit-scrollbar{width:6px}
        .network-list::-webkit-scrollbar-track{background:#1a1412}
        .network-list::-webkit-scrollbar-thumb{background:#3d2e24;border-radius:3px}
        .network-item{padding:14px 16px;border-bottom:1px solid #2d241e;cursor:pointer;transition:all 0.15s}
        .network-item:hover{background:#1a1412}
        .network-item:last-child{border-bottom:none}
        .network-item.selected{background:rgba(186,132,86,0.1);border-color:rgba(186,132,86,0.3)}
        .network-ssid{font-weight:500;color:#f5f0eb;font-size:15px;display:flex;align-items:center;gap:8px}
        .network-ssid .lock{color:#ba8456;font-size:12px}
        .network-rssi{font-size:12px;color:#7a6b5f;margin-top:4px}
        .signal-bars{display:inline-flex;gap:2px;margin-left:auto}
        .signal-bar{width:3px;background:#3d2e24;border-radius:1px}
        .signal-bar.active{background:#ba8456}
        .empty-state{text-align:center;padding:40px 20px;color:#5c4d42}
        .empty-state svg{width:48px;height:48px;margin-bottom:16px;opacity:0.5}
        .spinner{display:inline-block;width:18px;height:18px;border:2px solid rgba(255,255,255,0.3);border-top-color:#fff;border-radius:50%;animation:spin 0.6s linear infinite;margin-right:8px;vertical-align:middle}
        @keyframes spin{to{transform:rotate(360deg)}}
        .divider{height:1px;background:linear-gradient(90deg,transparent,#3d2e24,transparent);margin:24px 0}
    </style>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
</head>
<body>
    <div class="card">
        <svg class="logo" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg">
            <circle cx="50" cy="50" r="48" fill="url(#grad1)" stroke="#ba8456" stroke-width="2"/>
            <path d="M30 35C30 35 32 25 50 25C68 25 70 35 70 35V60C70 70 60 75 50 75C40 75 30 70 30 60V35Z" fill="#2d1f18" stroke="#ba8456" stroke-width="2"/>
            <path d="M70 40H75C80 40 82 45 82 50C82 55 80 60 75 60H70" stroke="#ba8456" stroke-width="2" fill="none"/>
            <ellipse cx="50" cy="35" rx="18" ry="6" fill="#ba8456" opacity="0.3"/>
            <path d="M40 50C42 55 48 58 50 58C52 58 58 55 60 50" stroke="#d5a071" stroke-width="2" stroke-linecap="round" opacity="0.6"/>
            <defs><linearGradient id="grad1" x1="0%" y1="0%" x2="100%" y2="100%"><stop offset="0%" stop-color="#1e1714"/><stop offset="100%" stop-color="#0d0a09"/></linearGradient></defs>
        </svg>
        
        <h1>BrewOS</h1>
        <p class="subtitle">Connect your espresso machine to WiFi</p>
        
        <div class="form-group">
            <label>Available Networks</label>
            <div id="networkList" class="network-list">
                <div class="empty-state">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M8.288 15.038a5.25 5.25 0 017.424 0M5.106 11.856c3.807-3.808 9.98-3.808 13.788 0M1.924 8.674c5.565-5.565 14.587-5.565 20.152 0M12.53 18.22l-.53.53-.53-.53a.75.75 0 011.06 0z"/>
                    </svg>
                    <p>Tap "Scan" to find networks</p>
                </div>
            </div>
        </div>
        
        <button id="scanBtn" class="btn btn-secondary" onclick="scanNetworks()">
            <span id="scanSpinner" class="spinner" style="display:none"></span>
            <span id="scanText">Scan for Networks</span>
        </button>
        
        <div class="divider"></div>
        
        <div class="form-group" id="passwordGroup" style="display:none">
            <label>WiFi Password</label>
            <input type="password" id="password" placeholder="Enter password">
        </div>
        
        <button id="connectBtn" class="btn" onclick="connectWiFi()" disabled>
            <span id="connectSpinner" class="spinner" style="display:none"></span>
            <span id="connectText">Connect to Network</span>
        </button>
        
        <div id="status" class="status"></div>
    </div>
    
    <script>
        let selectedSSID = '';
        
        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
        }
        
        function hideStatus() {
            document.getElementById('status').style.display = 'none';
        }
        
        function getSignalBars(rssi) {
            const bars = rssi > -50 ? 4 : rssi > -60 ? 3 : rssi > -70 ? 2 : 1;
            return Array(4).fill(0).map((_, i) => 
                `<div class="signal-bar${i < bars ? ' active' : ''}" style="height:${6 + i * 3}px"></div>`
            ).join('');
        }
        
        async function scanNetworks() {
            const btn = document.getElementById('scanBtn');
            const spinner = document.getElementById('scanSpinner');
            const text = document.getElementById('scanText');
            const list = document.getElementById('networkList');
            
            btn.disabled = true;
            spinner.style.display = 'inline-block';
            text.textContent = 'Scanning...';
            hideStatus();
            
            try {
                const response = await fetch('/api/wifi/networks');
                const data = await response.json();
                
                if (data.networks && data.networks.length > 0) {
                    list.innerHTML = '';
                    data.networks.sort((a,b) => b.rssi - a.rssi).forEach(network => {
                        const item = document.createElement('div');
                        item.className = 'network-item';
                        item.onclick = () => selectNetwork(network.ssid, network.secure, item);
                        item.innerHTML = `
                            <div class="network-ssid">
                                ${escapeHtml(network.ssid)}
                                ${network.secure ? '<span class="lock">🔒</span>' : ''}
                                <span class="signal-bars">${getSignalBars(network.rssi)}</span>
                            </div>
                            <div class="network-rssi">${network.rssi} dBm</div>
                        `;
                        list.appendChild(item);
                    });
                    showStatus(data.networks.length + ' networks found', 'success');
                } else {
                    list.innerHTML = '<div class="empty-state"><p>No networks found</p></div>';
                    showStatus('No networks found. Try again.', 'error');
                }
            } catch (error) {
                showStatus('Scan failed. Please try again.', 'error');
                list.innerHTML = '<div class="empty-state"><p>Scan failed</p></div>';
            }
            
            btn.disabled = false;
            spinner.style.display = 'none';
            text.textContent = 'Scan for Networks';
        }
        
        function selectNetwork(ssid, secure, element) {
            selectedSSID = ssid;
            document.getElementById('passwordGroup').style.display = secure ? 'block' : 'none';
            document.getElementById('connectBtn').disabled = false;
            
            document.querySelectorAll('.network-item').forEach(item => item.classList.remove('selected'));
            element.classList.add('selected');
            
            showStatus('Selected: ' + ssid, 'info');
        }
        
        async function connectWiFi() {
            if (!selectedSSID) {
                showStatus('Please select a network first', 'error');
                return;
            }
            
            const password = document.getElementById('password').value;
            const btn = document.getElementById('connectBtn');
            const spinner = document.getElementById('connectSpinner');
            const text = document.getElementById('connectText');
            
            btn.disabled = true;
            spinner.style.display = 'inline-block';
            text.textContent = 'Connecting...';
            hideStatus();
            
            try {
                const response = await fetch('/api/wifi/connect', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ssid: selectedSSID, password: password})
                });
                
                const data = await response.json();
                
                if (response.ok) {
                    showStatus('Connected! Redirecting to BrewOS...', 'success');
                    text.textContent = 'Connected!';
                    setTimeout(() => {
                        window.location.href = 'http://brewos.local';
                    }, 3000);
                } else {
                    showStatus(data.error || 'Connection failed', 'error');
                    btn.disabled = false;
                    spinner.style.display = 'none';
                    text.textContent = 'Connect to Network';
                }
            } catch (error) {
                showStatus('Connection error. Please try again.', 'error');
                btn.disabled = false;
                spinner.style.display = 'none';
                text.textContent = 'Connect to Network';
            }
        }
        
        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
        
        // Auto-scan on load
        window.onload = () => scanNetworks();
    </script>
</body>
</html>
)rawliteral";
        // Copy from PROGMEM to regular RAM for AsyncWebServer
        size_t htmlLen = strlen_P(html);
        char* htmlBuffer = (char*)heap_caps_malloc(htmlLen + 1, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (htmlBuffer) {
            strcpy_P(htmlBuffer, html);
            request->send(200, "text/html", htmlBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "text/plain", "Out of memory");
        }
    });
    
    // Root route - serve React app
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[WEB] / hit - serving index.html");
        request->send(LittleFS, "/index.html", "text/html");
    });
    
    // NOTE: serveStatic is registered at the END of setupRoutes() to ensure
    // API routes have priority over static file serving
    
    // Captive portal detection routes - redirect to lightweight setup page
    // Android/Chrome
    _server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    _server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    // Apple
    _server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    _server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    // Windows
    _server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    _server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    // Firefox
    _server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    // Generic captive portal check
    _server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/setup");
    });
    
    // API endpoints
    
    // Check if in AP mode (for WiFi setup detection)
    _server.on("/api/mode", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Delay serving if WiFi just connected (prevents PSRAM crashes)
        // Use const char* to avoid String allocation in PSRAM
        if (!_wifiManager.isAPMode() && !isWiFiReady()) {
            const char* error = "{\"error\":\"WiFi initializing, please wait\"}";
            request->send(503, "application/json", error);
            return;
        }
        
        // Use stack allocation to avoid PSRAM crashes
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<256> doc;
        #pragma GCC diagnostic pop
        doc["mode"] = "local";
        doc["apMode"] = _wifiManager.isAPMode();
        // Copy hostname to stack buffer to avoid PSRAM
        // Use a local scope to ensure String is destroyed before any other operations
        char hostnameBuf[64];
        {
            String hostnameStr = WiFi.getHostname();
            if (hostnameStr.length() > 0) {
                strncpy(hostnameBuf, hostnameStr.c_str(), sizeof(hostnameBuf) - 1);
                hostnameBuf[sizeof(hostnameBuf) - 1] = '\0';
            } else {
                strncpy(hostnameBuf, "brewos", sizeof(hostnameBuf) - 1);
                hostnameBuf[sizeof(hostnameBuf) - 1] = '\0';
            }
            // String destructor runs here at end of scope, before any other operations
        }
        doc["hostname"] = hostnameBuf;
        
        // Serialize JSON to buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // API info endpoint - provides version and feature detection for web UI compatibility
    // This is the primary endpoint for version negotiation between web UI and backend
    _server.on("/api/info", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Delay serving if WiFi just connected (prevents PSRAM crashes)
        // Use const char* to avoid String allocation in PSRAM
        if (!_wifiManager.isAPMode() && !isWiFiReady()) {
            const char* error = "{\"error\":\"WiFi initializing, please wait\"}";
            request->send(503, "application/json", error);
            return;
        }
        
        // Use stack allocation to avoid PSRAM crashes
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<1024> doc;
        #pragma GCC diagnostic pop
        
        // API version - increment ONLY for breaking changes to REST/WebSocket APIs
        // Web UI checks this to determine compatibility
        doc["apiVersion"] = 1;
        
        // Component versions
        doc["firmwareVersion"] = ESP32_VERSION;
        doc["webVersion"] = ESP32_VERSION;  // Web UI bundled with this firmware
        doc["protocolVersion"] = ECM_PROTOCOL_VERSION;
        
        // Pico version (if connected) - with safety check
        if (_picoUart.isConnected()) {
            doc["picoConnected"] = true;
            // Safely get Pico version - State might not be fully initialized
            const char* picoVer = State.getPicoVersion();
            if (picoVer && picoVer[0] != '\0') {
                doc["picoVersion"] = picoVer;
            }
        } else {
            doc["picoConnected"] = false;
        }
        
        // Mode detection
        doc["mode"] = "local";
        doc["apMode"] = _wifiManager.isAPMode();
        
        // Feature flags - granular capability detection
        // Web UI uses these to conditionally show/hide features
        JsonArray features = doc.createNestedArray("features");
        
        // Core features (always available)
        features.add("temperature_control");
        features.add("pressure_monitoring");
        features.add("power_monitoring");
        
        // Advanced features
        features.add("bbw");              // Brew-by-weight
        features.add("scale");            // BLE scale support
        features.add("mqtt");             // MQTT integration
        features.add("eco_mode");         // Eco mode
        features.add("statistics");       // Statistics tracking
        features.add("schedules");        // Schedule management
        
        // OTA features
        features.add("pico_ota");         // Pico firmware updates
        features.add("esp32_ota");        // ESP32 firmware updates
        
        // Debug features
        features.add("debug_console");    // Debug console
        features.add("protocol_debug");   // Protocol debugging
        
        // Device info - get MAC and hostname directly to avoid String allocations
        // WiFi.macAddress() and WiFi.getHostname() return String, but we copy immediately
        // to minimize PSRAM exposure
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char macBuf[18];  // MAC address format: "XX:XX:XX:XX:XX:XX" + null
        snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        // Get hostname - must use String but copy immediately to minimize PSRAM exposure
        // Use a local scope to ensure String is destroyed before any other operations
        char hostnameBuf[64];
        {
            String hostnameStr = WiFi.getHostname();
            if (hostnameStr.length() > 0) {
                strncpy(hostnameBuf, hostnameStr.c_str(), sizeof(hostnameBuf) - 1);
                hostnameBuf[sizeof(hostnameBuf) - 1] = '\0';
            } else {
                strncpy(hostnameBuf, "brewos", sizeof(hostnameBuf) - 1);
                hostnameBuf[sizeof(hostnameBuf) - 1] = '\0';
            }
            // String destructor runs here at end of scope, before any other operations
        }
        
        doc["deviceId"] = macBuf;
        doc["hostname"] = hostnameBuf;
        
        // Serialize JSON to buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    // ==========================================================================
    // Statistics API endpoints
    // ==========================================================================
    
    // Get full statistics
    _server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest* request) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        BrewOS::FullStatistics stats;
        Stats.getFullStatistics(stats);
        
        JsonObject obj = doc.to<JsonObject>();
        stats.toJson(obj);
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Get extended statistics with history data
    _server.on("/api/stats/extended", HTTP_GET, [](AsyncWebServerRequest* request) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        
        // Get full statistics
        BrewOS::FullStatistics stats;
        Stats.getFullStatistics(stats);
        
        JsonObject statsObj = doc["stats"].to<JsonObject>();
        stats.toJson(statsObj);
        
        // Weekly chart data
        JsonArray weeklyArr = doc["weekly"].to<JsonArray>();
        Stats.getWeeklyBrewChart(weeklyArr);
        
        // Hourly distribution
        JsonArray hourlyArr = doc["hourlyDistribution"].to<JsonArray>();
        Stats.getHourlyDistribution(hourlyArr);
        
        // Brew history (last 50)
        JsonArray brewArr = doc["brewHistory"].to<JsonArray>();
        Stats.getBrewHistory(brewArr, 50);
        
        // Power history (last 24 hours)
        JsonArray powerArr = doc["powerHistory"].to<JsonArray>();
        Stats.getPowerHistory(powerArr);
        
        // Daily history (last 30 days)
        JsonArray dailyArr = doc["dailyHistory"].to<JsonArray>();
        Stats.getDailyHistory(dailyArr);
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Get brew history
    _server.on("/api/stats/brews", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        
        // Check for limit parameter
        size_t limit = 50;
        if (request->hasParam("limit")) {
            limit = request->getParam("limit")->value().toInt();
            if (limit > 200) limit = 200;
        }
        
        Stats.getBrewHistory(arr, limit);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Get power history
    _server.on("/api/stats/power", HTTP_GET, [](AsyncWebServerRequest* request) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        JsonArray arr = doc.to<JsonArray>();
        Stats.getPowerHistory(arr);
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Reset statistics (with confirmation)
    _server.on("/api/stats/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        Stats.resetAll();
        broadcastLog("Statistics reset", static_cast<const char*>("warn"));
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    _server.on("/api/wifi/networks", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetWiFiNetworks(request);
    });
    
    _server.on("/api/wifi/connect", HTTP_POST, 
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetWiFi(request, data, len);
        }
    );
    
    _server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetConfig(request);
    });
    
    _server.on("/api/command", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleCommand(request, data, len);
        }
    );
    
    // OTA upload
    _server.on("/api/ota/upload", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Initial response - upload starting
            request->send(200, "application/json", "{\"status\":\"uploading\"}");
        },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, 
               uint8_t* data, size_t len, bool final) {
            handleOTAUpload(request, filename, index, data, len, final);
        }
    );
    
    _server.on("/api/ota/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleStartOTA(request);
    });
    
    _server.on("/api/pico/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _picoUart.resetPico();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Setup status endpoint (check if first-run wizard is complete)
    _server.on("/api/setup/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool complete = State.settings().system.setupComplete;
        char response[64];
        snprintf(response, sizeof(response), "{\"complete\":%s}", complete ? "true" : "false");
        request->send(200, "application/json", response);
    });
    
    // Setup complete endpoint (marks first-run wizard as done)
    // Note: No auth required - this endpoint is only accessible on local network
    // during initial device setup before WiFi is configured
    _server.on("/api/setup/complete", HTTP_POST, [](AsyncWebServerRequest* request) {
        // Only allow if not already completed (prevent re-triggering)
        if (State.settings().system.setupComplete) {
            request->send(200, "application/json", "{\"success\":true,\"alreadyComplete\":true}");
            return;
        }
        
        State.settings().system.setupComplete = true;
        State.saveSystemSettings();
        LOG_I("Setup wizard completed");
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    // MQTT endpoints
    _server.on("/api/mqtt/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetMQTTConfig(request);
    });
    
    _server.on("/api/mqtt/config", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetMQTTConfig(request, data, len);
        }
    );
    
    _server.on("/api/mqtt/test", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleTestMQTT(request);
    });
    
    // Brew-by-Weight settings
    _server.on("/api/scale/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        bbw_settings_t settings = brewByWeight ? brewByWeight->getSettings() : bbw_settings_t{};
        
        doc["target_weight"] = settings.target_weight;
        doc["dose_weight"] = settings.dose_weight;
        doc["stop_offset"] = settings.stop_offset;
        doc["auto_stop"] = settings.auto_stop;
        doc["auto_tare"] = settings.auto_tare;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    _server.on("/api/scale/settings", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            if (!doc["target_weight"].isNull()) {
                brewByWeight->setTargetWeight(doc["target_weight"].as<float>());
            }
            if (!doc["dose_weight"].isNull()) {
                brewByWeight->setDoseWeight(doc["dose_weight"].as<float>());
            }
            if (!doc["stop_offset"].isNull()) {
                brewByWeight->setStopOffset(doc["stop_offset"].as<float>());
            }
            if (!doc["auto_stop"].isNull()) {
                brewByWeight->setAutoStop(doc["auto_stop"].as<bool>());
            }
            if (!doc["auto_tare"].isNull()) {
                brewByWeight->setAutoTare(doc["auto_tare"].as<bool>());
            }
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    _server.on("/api/scale/state", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        bbw_state_t state = brewByWeight ? brewByWeight->getState() : bbw_state_t{};
        bbw_settings_t settings = brewByWeight ? brewByWeight->getSettings() : bbw_settings_t{};
        
        doc["active"] = state.active;
        doc["current_weight"] = state.current_weight;
        doc["target_weight"] = settings.target_weight;
        doc["progress"] = brewByWeight ? brewByWeight->getProgress() : 0.0f;
        doc["ratio"] = brewByWeight ? brewByWeight->getCurrentRatio() : 0.0f;
        doc["target_reached"] = state.target_reached;
        doc["stop_signaled"] = state.stop_signaled;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    _server.on("/api/scale/tare", HTTP_POST, [](AsyncWebServerRequest* request) {
        scaleManager->tare();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Scale connection status
    _server.on("/api/scale/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        scale_state_t state = scaleManager ? scaleManager->getState() : scale_state_t{};
        
        doc["connected"] = scaleManager ? scaleManager->isConnected() : false;
        doc["scanning"] = scaleManager ? scaleManager->isScanning() : false;
        doc["name"] = scaleManager ? scaleManager->getScaleName() : "";
        doc["type"] = scaleManager ? (int)scaleManager->getScaleType() : 0;
        doc["type_name"] = scaleManager ? getScaleTypeName(scaleManager->getScaleType()) : "";
        doc["weight"] = state.weight;
        doc["stable"] = state.stable;
        doc["flow_rate"] = state.flow_rate;
        doc["battery"] = state.battery_percent;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Start BLE scale scan
    _server.on("/api/scale/scan", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (scaleManager && scaleManager->isScanning()) {
            request->send(400, "application/json", "{\"error\":\"Already scanning\"}");
            return;
        }
        if (scaleManager && scaleManager->isConnected()) {
            scaleManager->disconnect();
        }
        scaleManager->clearDiscovered();
        scaleManager->startScan(15000);  // 15 second scan
        broadcastLog("BLE scale scan started", "info");
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Scanning...\"}");
    });
    
    // Stop BLE scan
    _server.on("/api/scale/scan/stop", HTTP_POST, [](AsyncWebServerRequest* request) {
        scaleManager->stopScan();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Get discovered scales
    _server.on("/api/scale/devices", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray devices = doc["devices"].to<JsonArray>();
        
        static const std::vector<scale_info_t> empty_devices;
        const auto& discovered = scaleManager ? scaleManager->getDiscoveredScales() : empty_devices;
        for (size_t i = 0; i < discovered.size(); i++) {
            JsonObject device = devices.add<JsonObject>();
            device["index"] = i;
            device["name"] = discovered[i].name;
            device["address"] = discovered[i].address;
            device["type"] = (int)discovered[i].type;
            device["type_name"] = getScaleTypeName(discovered[i].type);
            device["rssi"] = discovered[i].rssi;
        }
        
        doc["scanning"] = scaleManager ? scaleManager->isScanning() : false;
        doc["count"] = discovered.size();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Connect to scale by address or index
    _server.on("/api/scale/connect", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            bool success = false;
            
            if (!doc["address"].isNull()) {
                // Connect by address
                const char* addr = doc["address"].as<const char*>();
                if (addr && strlen(addr) > 0) {
                    success = scaleManager ? scaleManager->connect(addr) : false;
                }
            } else if (!doc["index"].isNull()) {
                // Connect by index from discovered list
                int idx = doc["index"].as<int>();
                success = scaleManager ? scaleManager->connectByIndex(idx) : false;
            } else {
                // Try to reconnect to saved scale
                success = scaleManager ? scaleManager->connect(nullptr) : false;
            }
            
            if (success) {
                broadcastLog("Connecting to scale...", "info");
                request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Connecting...\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Connection failed\"}");
            }
        }
    );
    
    // Disconnect from scale
    _server.on("/api/scale/disconnect", HTTP_POST, [](AsyncWebServerRequest* request) {
        scaleManager->disconnect();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Forget saved scale
    _server.on("/api/scale/forget", HTTP_POST, [this](AsyncWebServerRequest* request) {
        scaleManager->forgetScale();
        broadcastLog("Scale forgotten", "info");
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Timer control (for scales that support it)
    _server.on("/api/scale/timer/start", HTTP_POST, [](AsyncWebServerRequest* request) {
        scaleManager->startTimer();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    _server.on("/api/scale/timer/stop", HTTP_POST, [](AsyncWebServerRequest* request) {
        scaleManager->stopTimer();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    _server.on("/api/scale/timer/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        scaleManager->resetTimer();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // ==========================================================================
    // Schedule API endpoints
    // ==========================================================================
    
    // Get all schedules
    _server.on("/api/schedules", HTTP_GET, [](AsyncWebServerRequest* request) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        JsonObject obj = doc.to<JsonObject>();
        State.settings().schedule.toJson(obj);
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Add a new schedule
    _server.on("/api/schedules", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            BrewOS::ScheduleEntry entry;
            entry.fromJson(doc.as<JsonObjectConst>());
            
            uint8_t newId = State.addSchedule(entry);
            if (newId > 0) {
                JsonDocument resp;
                resp["status"] = "ok";
                resp["id"] = newId;
                String response;
                serializeJson(resp, response);
                request->send(200, "application/json", response);
                broadcastLog("Schedule added: %s", entry.name);
            } else {
                request->send(400, "application/json", "{\"error\":\"Max schedules reached\"}");
            }
        }
    );
    
    // Update a schedule
    _server.on("/api/schedules/update", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            uint8_t id = doc["id"] | 0;
            if (id == 0) {
                request->send(400, "application/json", "{\"error\":\"Missing schedule ID\"}");
                return;
            }
            
            BrewOS::ScheduleEntry entry;
            entry.fromJson(doc.as<JsonObjectConst>());
            
            if (State.updateSchedule(id, entry)) {
                request->send(200, "application/json", "{\"status\":\"ok\"}");
                broadcastLog("Schedule updated: %s", entry.name);
            } else {
                request->send(404, "application/json", "{\"error\":\"Schedule not found\"}");
            }
        }
    );
    
    // Delete a schedule
    _server.on("/api/schedules/delete", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            uint8_t id = doc["id"] | 0;
            if (id == 0) {
                request->send(400, "application/json", "{\"error\":\"Missing schedule ID\"}");
                return;
            }
            
            if (State.removeSchedule(id)) {
                request->send(200, "application/json", "{\"status\":\"ok\"}");
                broadcastLog("Schedule deleted", "info");
            } else {
                request->send(404, "application/json", "{\"error\":\"Schedule not found\"}");
            }
        }
    );
    
    // Enable/disable a schedule
    _server.on("/api/schedules/toggle", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            uint8_t id = doc["id"] | 0;
            bool enabled = doc["enabled"] | false;
            
            if (id == 0) {
                request->send(400, "application/json", "{\"error\":\"Missing schedule ID\"}");
                return;
            }
            
            if (State.enableSchedule(id, enabled)) {
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Schedule not found\"}");
            }
        }
    );
    
    // Auto power-off settings
    _server.on("/api/schedules/auto-off", HTTP_GET, [](AsyncWebServerRequest* request) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        doc["enabled"] = State.getAutoPowerOffEnabled();
        doc["minutes"] = State.getAutoPowerOffMinutes();
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    _server.on("/api/schedules/auto-off", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            bool enabled = doc["enabled"] | false;
            uint16_t minutes = doc["minutes"] | 60;
            
            State.setAutoPowerOff(enabled, minutes);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
            broadcastLog("Auto power-off: %s (%d min)", enabled ? "enabled" : "disabled", minutes);
        }
    );
    
    // ==========================================================================
    // Time/NTP API endpoints
    // ==========================================================================
    
    // Get time status and settings
    _server.on("/api/time", HTTP_GET, [this](AsyncWebServerRequest* request) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        
        // Current time status
        TimeStatus timeStatus = _wifiManager.getTimeStatus();
        doc["synced"] = timeStatus.ntpSynced;
        doc["currentTime"] = timeStatus.currentTime;
        doc["timezone"] = timeStatus.timezone;
        doc["utcOffset"] = timeStatus.utcOffset;
        
        // Settings
        JsonObject settings = doc["settings"].to<JsonObject>();
        State.settings().time.toJson(settings);
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Update time settings
    _server.on("/api/time", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            auto& timeSettings = State.settings().time;
            
            if (!doc["useNTP"].isNull()) timeSettings.useNTP = doc["useNTP"].as<bool>();
            if (!doc["ntpServer"].isNull()) strncpy(timeSettings.ntpServer, doc["ntpServer"].as<const char*>(), sizeof(timeSettings.ntpServer) - 1);
            if (!doc["utcOffsetMinutes"].isNull()) timeSettings.utcOffsetMinutes = doc["utcOffsetMinutes"].as<int16_t>();
            if (!doc["dstEnabled"].isNull()) timeSettings.dstEnabled = doc["dstEnabled"].as<bool>();
            if (!doc["dstOffsetMinutes"].isNull()) timeSettings.dstOffsetMinutes = doc["dstOffsetMinutes"].as<int16_t>();
            
            State.saveTimeSettings();
            
            // Apply new NTP settings
            _wifiManager.configureNTP(
                timeSettings.ntpServer,
                timeSettings.utcOffsetMinutes,
                timeSettings.dstEnabled,
                timeSettings.dstOffsetMinutes
            );
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
            broadcastLog("Time settings updated", "info");
        }
    );
    
    // Force NTP sync
    _server.on("/api/time/sync", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _wifiManager.syncNTP();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"NTP sync initiated\"}");
        broadcastLog("NTP sync initiated", "info");
    });
    
    // Temperature control endpoints
    _server.on("/api/temp/brew", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            float temp = doc["temp"] | 0.0f;
            if (temp < 80.0f || temp > 105.0f) {
                request->send(400, "application/json", "{\"error\":\"Temperature out of range (80-105°C)\"}");
                return;
            }
            
            // Send to Pico
            uint8_t payload[5];
            payload[0] = 0x01;  // Brew boiler ID
            memcpy(&payload[1], &temp, sizeof(float));
            if (_picoUart.sendCommand(MSG_CMD_SET_TEMP, payload, 5)) {
                broadcastLog("Brew temp set to %.1f°C", temp);
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Failed to send command\"}");
            }
        }
    );
    
    _server.on("/api/temp/steam", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            float temp = doc["temp"] | 0.0f;
            if (temp < 120.0f || temp > 160.0f) {
                request->send(400, "application/json", "{\"error\":\"Temperature out of range (120-160°C)\"}");
                return;
            }
            
            // Send to Pico
            uint8_t payload[5];
            payload[0] = 0x02;  // Steam boiler ID
            memcpy(&payload[1], &temp, sizeof(float));
            if (_picoUart.sendCommand(MSG_CMD_SET_TEMP, payload, 5)) {
                broadcastLog("Steam temp set to %.1f°C", temp);
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Failed to send command\"}");
            }
        }
    );
    
    // Machine mode control
    _server.on("/api/mode", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            String mode = doc["mode"] | "";
            uint8_t cmd = 0;
            
            if (mode == "on" || mode == "ready") {
                cmd = 0x01;  // Turn on
            } else if (mode == "off" || mode == "standby") {
                cmd = 0x00;  // Turn off
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid mode (use: on, off, ready, standby)\"}");
                return;
            }
            
            if (_picoUart.sendCommand(MSG_CMD_MODE, &cmd, 1)) {
                broadcastLog("Machine mode set to: %s", mode.c_str());
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Failed to send command\"}");
            }
        }
    );
    
    // Cloud status endpoint
    _server.on("/api/cloud/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        auto& cloudSettings = State.settings().cloud;
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        doc["enabled"] = cloudSettings.enabled;
        doc["connected"] = _cloudConnection ? _cloudConnection->isConnected() : false;
        doc["serverUrl"] = cloudSettings.serverUrl;
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Push notification preferences endpoint (GET)
    _server.on("/api/push/preferences", HTTP_GET, [this](AsyncWebServerRequest* request) {
        auto& notifSettings = State.settings().notifications;
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        doc["machineReady"] = notifSettings.machineReady;
        doc["waterEmpty"] = notifSettings.waterEmpty;
        doc["descaleDue"] = notifSettings.descaleDue;
        doc["serviceDue"] = notifSettings.serviceDue;
        doc["backflushDue"] = notifSettings.backflushDue;
        doc["machineError"] = notifSettings.machineError;
        doc["picoOffline"] = notifSettings.picoOffline;
        doc["scheduleTriggered"] = notifSettings.scheduleTriggered;
        doc["brewComplete"] = notifSettings.brewComplete;
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // Push notification preferences endpoint (POST)
    _server.on(
        "/api/push/preferences",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data, len);
                
                if (error) {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    return;
                }
                
                auto& notifSettings = State.settings().notifications;
                
                if (!doc["machineReady"].isNull()) notifSettings.machineReady = doc["machineReady"];
                if (!doc["waterEmpty"].isNull()) notifSettings.waterEmpty = doc["waterEmpty"];
                if (!doc["descaleDue"].isNull()) notifSettings.descaleDue = doc["descaleDue"];
                if (!doc["serviceDue"].isNull()) notifSettings.serviceDue = doc["serviceDue"];
                if (!doc["backflushDue"].isNull()) notifSettings.backflushDue = doc["backflushDue"];
                if (!doc["machineError"].isNull()) notifSettings.machineError = doc["machineError"];
                if (!doc["picoOffline"].isNull()) notifSettings.picoOffline = doc["picoOffline"];
                if (!doc["scheduleTriggered"].isNull()) notifSettings.scheduleTriggered = doc["scheduleTriggered"];
                if (!doc["brewComplete"].isNull()) notifSettings.brewComplete = doc["brewComplete"];
                
                State.saveNotificationSettings();
                
                request->send(200, "application/json", "{\"success\":true}");
            }
        }
    );
    
    // Pairing API endpoints
    _server.on("/api/pairing/qr", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Check if cloud is enabled
        auto& cloudSettings = State.settings().cloud;
        if (!cloudSettings.enabled || !_pairingManager) {
            request->send(503, "application/json", "{\"error\":\"Cloud integration not enabled\"}");
            return;
        }
        
        // Generate a new token if needed
        if (!_pairingManager->isTokenValid()) {
            _pairingManager->generateToken();
        }
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        doc["deviceId"] = _pairingManager->getDeviceId();
        doc["token"] = _pairingManager->getCurrentToken();
        doc["url"] = _pairingManager->getPairingUrl();
        doc["expiresIn"] = (_pairingManager->getTokenExpiry() - millis()) / 1000;
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    _server.on("/api/pairing/refresh", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Check if cloud is enabled
        auto& cloudSettings = State.settings().cloud;
        if (!cloudSettings.enabled || !_pairingManager) {
            request->send(503, "application/json", "{\"error\":\"Cloud integration not enabled\"}");
            return;
        }
        
        _pairingManager->generateToken();
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        doc["deviceId"] = _pairingManager->getDeviceId();
        doc["token"] = _pairingManager->getCurrentToken();
        doc["url"] = _pairingManager->getPairingUrl();
        doc["expiresIn"] = 600;  // 10 minutes
        
        
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
    });
    
    // ==========================================================================
    // Diagnostics API endpoints
    // ==========================================================================
    
    // Run all diagnostic tests
    _server.on("/api/diagnostics/run", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Send command to Pico to run all diagnostics
        uint8_t payload[1] = { 0x00 };  // DIAG_TEST_ALL
        if (_picoUart.sendCommand(MSG_CMD_DIAGNOSTICS, payload, 1)) {
            broadcastLog("Running hardware diagnostics...", "info");
            request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Diagnostics started\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"Failed to send diagnostic command\"}");
        }
    });
    
    // Run a single diagnostic test
    _server.on("/api/diagnostics/test", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            uint8_t testId = doc["testId"] | 0;
            uint8_t payload[1] = { testId };
            
            if (_picoUart.sendCommand(MSG_CMD_DIAGNOSTICS, payload, 1)) {
                broadcastLog("Running diagnostic test %d", testId);
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Failed to send command\"}");
            }
        }
    );
    
    // ==========================================================================
    // Web Assets OTA Update
    // ==========================================================================
    
    // Start web OTA - cleans old assets and prepares for upload
    // This is called once at the beginning of a web update session
    _server.on("/api/ota/web/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        LOG_I("Starting web OTA - cleaning old assets...");
        int deleted = 0;
        
        // Remove files in /assets directory (hashed JS/CSS bundles)
        if (LittleFS.exists("/assets")) {
            File assets = LittleFS.open("/assets");
            File file = assets.openNextFile();
            std::vector<String> toDelete;
            while (file) {
                toDelete.push_back(String("/assets/") + file.name());
                file = assets.openNextFile();
            }
            for (const String& path : toDelete) {
                if (LittleFS.remove(path)) {
                    deleted++;
                }
            }
            LittleFS.rmdir("/assets");
        }
        
        // Remove root-level web files (keep system config files)
        const char* webFiles[] = {"index.html", "favicon.svg", "favicon.ico", "logo.png", 
                                   "logo-icon.svg", "manifest.json", "sw.js", "version-manifest.json"};
        for (const char* filename : webFiles) {
            String path = String("/") + filename;
            if (LittleFS.exists(path) && LittleFS.remove(path)) {
                deleted++;
            }
        }
        
        // Recreate assets directory
        LittleFS.mkdir("/assets");
        
        LOG_I("Cleaned %d old web files, ready for upload", deleted);
        
        char response[64];
        snprintf(response, sizeof(response), "{\"cleaned\":%d,\"status\":\"ready\"}", deleted);
        request->send(200, "application/json", response);
    });
    
    // Upload web asset file - called for each file in the web bundle
    _server.on("/api/ota/web/upload", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, 
               uint8_t* data, size_t len, bool final) {
            static File uploadFile;
            String path = "/" + filename;
            
            if (index == 0) {
                uploadFile = LittleFS.open(path, "w");
                if (!uploadFile) {
                    LOG_E("Failed to open %s for writing", path.c_str());
                    return;
                }
            }
            
            if (uploadFile && len > 0) {
                uploadFile.write(data, len);
            }
            
            if (final && uploadFile) {
                uploadFile.close();
                LOG_D("Web OTA: %s (%u bytes)", path.c_str(), (unsigned)(index + len));
            }
        }
    );
    
    // Complete web OTA - logs completion
    _server.on("/api/ota/web/complete", HTTP_POST, [this](AsyncWebServerRequest* request) {
        size_t used = LittleFS.usedBytes();
        size_t total = LittleFS.totalBytes();
        LOG_I("Web OTA complete. Filesystem: %uKB / %uKB", (unsigned)(used/1024), (unsigned)(total/1024));
        broadcastLog("Web update complete");
        
        char response[96];
        snprintf(response, sizeof(response), 
            "{\"status\":\"complete\",\"used\":%u,\"total\":%u}", 
            (unsigned)used, (unsigned)total);
        request->send(200, "application/json", response);
    });
    
    // Serve static files from LittleFS (React app assets: JS, CSS, images, etc.)
    // This is registered AFTER all API routes to ensure API endpoints have priority
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // SPA fallback - serve index.html for all non-API routes (React Router handles client-side routing)
    // This is critical for SPA apps: routes like /stats, /settings, etc. are handled by React Router
    _server.onNotFound([](AsyncWebServerRequest* request) {
        String url = request->url();
        
        // API routes should return 404 if not found
        if (url.startsWith("/api/")) {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
            return;
        }
        
        // For all other routes, serve index.html (SPA fallback)
        // React Router will handle the routing client-side
        Serial.printf("[WEB] SPA fallback: %s -> index.html\n", url.c_str());
        request->send(LittleFS, "/index.html", "text/html");
    });
    
    LOG_I("Routes setup complete");
}

void WebServer::handleGetStatus(AsyncWebServerRequest* request) {
    // Delay serving if WiFi just connected (prevents PSRAM crashes)
    // Use const char* to avoid String allocation in PSRAM
    if (!_wifiManager.isAPMode() && !isWiFiReady()) {
        const char* error = "{\"error\":\"WiFi initializing, please wait\"}";
        request->send(503, "application/json", error);
        return;
    }
    
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<2048> doc;
    #pragma GCC diagnostic pop
    
    // WiFi status
    // CRITICAL: Copy WiFiStatus Strings to stack buffers immediately to avoid PSRAM pointer issues
    // WiFiStatus contains String fields that might be allocated in PSRAM
    WiFiStatus wifi = _wifiManager.getStatus();
    
    // Copy Strings to stack buffers (internal RAM) before using them
    char ssidBuf[64];
    char ipBuf[16];
    char gatewayBuf[16];
    char subnetBuf[16];
    char dns1Buf[16];
    char dns2Buf[16];
    
    strncpy(ssidBuf, wifi.ssid.c_str(), sizeof(ssidBuf) - 1);
    ssidBuf[sizeof(ssidBuf) - 1] = '\0';
    strncpy(ipBuf, wifi.ip.c_str(), sizeof(ipBuf) - 1);
    ipBuf[sizeof(ipBuf) - 1] = '\0';
    strncpy(gatewayBuf, wifi.gateway.c_str(), sizeof(gatewayBuf) - 1);
    gatewayBuf[sizeof(gatewayBuf) - 1] = '\0';
    strncpy(subnetBuf, wifi.subnet.c_str(), sizeof(subnetBuf) - 1);
    subnetBuf[sizeof(subnetBuf) - 1] = '\0';
    strncpy(dns1Buf, wifi.dns1.c_str(), sizeof(dns1Buf) - 1);
    dns1Buf[sizeof(dns1Buf) - 1] = '\0';
    strncpy(dns2Buf, wifi.dns2.c_str(), sizeof(dns2Buf) - 1);
    dns2Buf[sizeof(dns2Buf) - 1] = '\0';
    
    doc["wifi"]["mode"] = (int)wifi.mode;
    doc["wifi"]["ssid"] = ssidBuf;
    doc["wifi"]["ip"] = ipBuf;
    doc["wifi"]["rssi"] = wifi.rssi;
    doc["wifi"]["configured"] = wifi.configured;
    doc["wifi"]["staticIp"] = wifi.staticIp;
    doc["wifi"]["gateway"] = gatewayBuf;
    doc["wifi"]["subnet"] = subnetBuf;
    doc["wifi"]["dns1"] = dns1Buf;
    doc["wifi"]["dns2"] = dns2Buf;
    
    // Pico status
    doc["pico"]["connected"] = _picoUart.isConnected();
    doc["pico"]["packetsReceived"] = _picoUart.getPacketsReceived();
    doc["pico"]["packetErrors"] = _picoUart.getPacketErrors();
    
    // ESP32 status
    doc["esp32"]["uptime"] = millis();
    doc["esp32"]["freeHeap"] = ESP.getFreeHeap();
    doc["esp32"]["version"] = ESP32_VERSION;
    
    // MQTT status
    doc["mqtt"]["enabled"] = _mqttClient.getConfig().enabled;
    doc["mqtt"]["connected"] = _mqttClient.isConnected();
    // Copy MQTT status string to stack buffer to avoid PSRAM
    String mqttStatusStr = _mqttClient.getStatusString();
    char mqttStatusBuf[32];
    strncpy(mqttStatusBuf, mqttStatusStr.c_str(), sizeof(mqttStatusBuf) - 1);
    mqttStatusBuf[sizeof(mqttStatusBuf) - 1] = '\0';
    doc["mqtt"]["status"] = mqttStatusBuf;
    
    // Scale status
    doc["scale"]["connected"] = scaleManager ? scaleManager->isConnected() : false;
    doc["scale"]["scanning"] = scaleManager ? scaleManager->isScanning() : false;
    // Copy scale name to stack buffer to avoid PSRAM
    String scaleNameStr = scaleManager ? scaleManager->getScaleName() : "";
    char scaleNameBuf[64];
    strncpy(scaleNameBuf, scaleNameStr.c_str(), sizeof(scaleNameBuf) - 1);
    scaleNameBuf[sizeof(scaleNameBuf) - 1] = '\0';
    doc["scale"]["name"] = scaleNameBuf;
    if (scaleManager && scaleManager->isConnected()) {
        scale_state_t scaleState = scaleManager->getState();
        doc["scale"]["weight"] = scaleState.weight;
        doc["scale"]["flow_rate"] = scaleState.flow_rate;
        doc["scale"]["stable"] = scaleState.stable;
    }
    
    // WebSocket clients
    doc["clients"] = getClientCount();
    
    // Setup status
    doc["setupComplete"] = State.settings().system.setupComplete;
    
    // Serialize JSON to buffer in internal RAM (not PSRAM)
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        request->send(200, "application/json", jsonBuffer);
        // Buffer will be freed by AsyncWebServer after response
    } else {
        request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
    }
}

// Static cache for async WiFi scan results
static bool _scanInProgress = false;
static bool _scanResultsReady = false;
static int _cachedNetworkCount = 0;
static unsigned long _lastScanTime = 0;
static const unsigned long SCAN_CACHE_TIMEOUT_MS = 30000;  // Cache results for 30 seconds

void WebServer::handleGetWiFiNetworks(AsyncWebServerRequest* request) {
    // Use cached results if available and fresh
    unsigned long now = millis();
    
    // If we have fresh cached results, return them immediately
    if (_scanResultsReady && (now - _lastScanTime < SCAN_CACHE_TIMEOUT_MS)) {
        LOG_I("Returning cached WiFi scan results (%d networks)", _cachedNetworkCount);
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        JsonArray networks = doc["networks"].to<JsonArray>();
        
        int n = WiFi.scanComplete();
        if (n > 0) {
            int maxNetworks = (n > 20) ? 20 : n;
            for (int i = 0; i < maxNetworks; i++) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() > 0) {
                    JsonObject network = networks.add<JsonObject>();
                    network["ssid"] = ssid;
                    network["rssi"] = WiFi.RSSI(i);
                    network["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                }
            }
        }
        
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
        return;
    }
    
    // Check if async scan is complete
    int scanResult = WiFi.scanComplete();
    
    if (scanResult == WIFI_SCAN_RUNNING) {
        // Scan still in progress - return status
        LOG_I("WiFi scan in progress...");
        request->send(202, "application/json", "{\"status\":\"scanning\",\"networks\":[]}");
        return;
    }
    
    if (scanResult >= 0) {
        // Scan complete - return results
        LOG_I("WiFi scan complete, found %d networks", scanResult);
        _scanResultsReady = true;
        _cachedNetworkCount = scanResult;
        _lastScanTime = now;
        _scanInProgress = false;
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
        JsonArray networks = doc["networks"].to<JsonArray>();
        
        int maxNetworks = (scanResult > 20) ? 20 : scanResult;
        for (int i = 0; i < maxNetworks; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() > 0) {
                JsonObject network = networks.add<JsonObject>();
                network["ssid"] = ssid;
                network["rssi"] = WiFi.RSSI(i);
                network["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            }
        }
        
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
        return;
    }
    
    // No scan in progress and no results - start async scan
    LOG_I("Starting async WiFi scan...");
    _scanInProgress = true;
    _scanResultsReady = false;
    
    // Switch to AP+STA mode if in pure AP mode
    bool wasAPMode = _wifiManager.isAPMode();
    if (wasAPMode && WiFi.getMode() == WIFI_AP) {
        WiFi.mode(WIFI_AP_STA);
        delay(100);  // Short delay for mode switch
    }
    
    // Clear previous results and start ASYNC scan (non-blocking!)
    WiFi.scanDelete();
    WiFi.scanNetworks(true, false);  // async=true, show_hidden=false
    
    // Return "scanning" status - client should poll again
    request->send(202, "application/json", "{\"status\":\"scanning\",\"networks\":[]}");
}

void WebServer::handleSetWiFi(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<512> doc;
    #pragma GCC diagnostic pop
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    
    if (_wifiManager.setCredentials(ssid, password)) {
        // Send response first - connection will happen in loop() after delay
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Connecting...\"}");
        
        // Set flag to trigger connection in the next loop iteration
        // This gives time for the HTTP response to be fully sent
        _pendingWiFiConnect = true;
    } else {
        request->send(400, "application/json", "{\"error\":\"Invalid credentials\"}");
    }
}

void WebServer::handleGetConfig(AsyncWebServerRequest* request) {
    // Request config from Pico
    _picoUart.requestConfig();
    
    // Return acknowledgment (actual config will come via WebSocket)
    request->send(200, "application/json", "{\"status\":\"requested\"}");
}

void WebServer::handleCommand(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String cmd = doc["cmd"] | "";
    
    if (cmd == "ping") {
        _picoUart.sendPing();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } 
    else if (cmd == "getConfig") {
        _picoUart.requestConfig();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
    else {
        request->send(400, "application/json", "{\"error\":\"Unknown command\"}");
    }
}

void WebServer::handleOTAUpload(AsyncWebServerRequest* request, const String& filename,
                                size_t index, uint8_t* data, size_t len, bool final) {
    static File otaFile;
    static size_t totalSize = 0;
    static size_t uploadedSize = 0;
    
    if (index == 0) {
        LOG_I("OTA upload started: %s", filename.c_str());
        totalSize = request->contentLength();
        uploadedSize = 0;
        
        // Delete old firmware if exists
        if (LittleFS.exists(OTA_FILE_PATH)) {
            LittleFS.remove(OTA_FILE_PATH);
        }
        
        otaFile = LittleFS.open(OTA_FILE_PATH, "w");
        if (!otaFile) {
            LOG_E("Failed to open OTA file for writing");
            request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
            return;
        }
    }
    
    if (otaFile && len > 0) {
        size_t written = otaFile.write(data, len);
        if (written != len) {
            LOG_E("Failed to write all data: %d/%d", written, len);
        }
        uploadedSize += written;
        
        // Report progress every 10%
        static size_t lastProgress = 0;
        size_t progress = (uploadedSize * 100) / totalSize;
        if (progress >= lastProgress + 10) {
            lastProgress = progress;
            JsonDocument doc;
            doc["type"] = "ota_progress";
            doc["stage"] = "upload";
            doc["progress"] = progress;
            doc["uploaded"] = uploadedSize;
            doc["total"] = totalSize;
            
            // Use heap_caps_malloc to avoid PSRAM
            size_t jsonSize = measureJson(doc) + 1;
            char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
            if (jsonBuffer) {
                serializeJson(doc, jsonBuffer, jsonSize);
                _ws.textAll(jsonBuffer);
                free(jsonBuffer);
            }
            LOG_I("Upload progress: %d%% (%d/%d bytes)", progress, uploadedSize, totalSize);
        }
    }
    
    if (final) {
        if (otaFile) {
            otaFile.close();
        }
        LOG_I("OTA upload complete: %d bytes", uploadedSize);
        
        // Verify file size
        File verifyFile = LittleFS.open(OTA_FILE_PATH, "r");
        bool uploadSuccess = true;
        if (verifyFile) {
            size_t fileSize = verifyFile.size();
            verifyFile.close();
            
            if (fileSize != uploadedSize) {
                LOG_E("File size mismatch: expected %d, got %d", uploadedSize, fileSize);
                broadcastLog("Upload failed: file size mismatch", "error");
                uploadSuccess = false;
            }
        } else {
            LOG_E("Failed to verify uploaded file");
            broadcastLog("Upload failed: file verification error", "error");
            uploadSuccess = false;
        }
        
        // Notify clients using stack allocation
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<256> finalDoc;
        #pragma GCC diagnostic pop
        finalDoc["type"] = "ota_progress";
        finalDoc["stage"] = "upload";
        finalDoc["progress"] = uploadSuccess ? 100 : 0;
        finalDoc["uploaded"] = uploadedSize;
        finalDoc["total"] = totalSize;
        finalDoc["success"] = uploadSuccess;
        
        size_t jsonSize = measureJson(finalDoc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
        if (jsonBuffer) {
            serializeJson(finalDoc, jsonBuffer, jsonSize);
            _ws.textAll(jsonBuffer);
            free(jsonBuffer);
        }
        
        if (uploadSuccess) {
            broadcastLog("Firmware uploaded: %zu bytes", uploadedSize);
        }
    }
}

void WebServer::handleStartOTA(AsyncWebServerRequest* request) {
    // Check if firmware file exists
    if (!LittleFS.exists(OTA_FILE_PATH)) {
        request->send(400, "application/json", "{\"error\":\"No firmware uploaded\"}");
        return;
    }
    
    File firmwareFile = LittleFS.open(OTA_FILE_PATH, "r");
    if (!firmwareFile) {
        request->send(500, "application/json", "{\"error\":\"Failed to open firmware file\"}");
        return;
    }
    
    size_t firmwareSize = firmwareFile.size();
    if (firmwareSize == 0 || firmwareSize > OTA_MAX_SIZE) {
        firmwareFile.close();
        request->send(400, "application/json", "{\"error\":\"Invalid firmware size\"}");
        return;
    }
    
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Starting OTA...\"}");
    
    broadcastLog("Starting Pico firmware update...", "info");
    
    // Step 1: Send bootloader command via UART (serial bootloader - preferred method)
    broadcastLog("Sending bootloader command to Pico...", "info");
    if (!_picoUart.sendCommand(MSG_CMD_BOOTLOADER, nullptr, 0)) {
        broadcastLog("Failed to send bootloader command", "error");
        firmwareFile.close();
        return;
    }
    
    // Step 2: Wait for bootloader ACK (0xAA 0x55)
    // The bootloader sends this ACK to confirm it's ready to receive firmware
    broadcastLog("Waiting for bootloader ACK...", "info");
    if (!_picoUart.waitForBootloaderAck(2000)) {
        broadcastLog("Bootloader ACK timeout - bootloader may not be ready", "error");
        firmwareFile.close();
        return;
    }
    broadcastLog("Bootloader ACK received, ready to stream firmware", "info");
    
    // Step 3: Stream firmware
    broadcastLog("Streaming firmware to Pico...", "info");
    bool success = streamFirmwareToPico(firmwareFile, firmwareSize);
    
    firmwareFile.close();
    
    if (!success) {
        broadcastLog("Firmware update failed", "error");
        // Fallback: Try hardware bootloader entry
        broadcastLog("Attempting hardware bootloader entry (fallback)...", "info");
        _picoUart.enterBootloader();
        delay(500);
        // Note: Hardware bootloader entry requires different protocol (USB bootloader)
        // This is a fallback for recovery only
        return;
    }
    
    // Step 4: Reset Pico to boot new firmware
    delay(1000);
    broadcastLog("Resetting Pico...", "info");
    _picoUart.resetPico();
    
    broadcastLog("Firmware update complete. Pico should boot with new firmware.", "info");
}

// WebSocket event handler for AsyncWebSocket (ESP32Async library)
void WebServer::handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_DISCONNECT:
            LOG_I("WebSocket client %u disconnected", client->id());
            break;
            
        case WS_EVT_CONNECT:
            LOG_I("WebSocket client %u connected from %s", client->id(), client->remoteIP().toString().c_str());
            broadcastLog("Client connected");
            break;
            
        case WS_EVT_DATA:
            {
                AwsFrameInfo* info = (AwsFrameInfo*)arg;
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    // Complete text message
                    handleWsMessage(client->id(), data, len);
                }
            }
            break;
            
        case WS_EVT_PONG:
            // Response to our ping
            break;
            
        case WS_EVT_ERROR:
            LOG_E("WebSocket error on client %u", client->id());
            break;
    }
}

void WebServer::handleWsMessage(uint32_t clientNum, uint8_t* payload, size_t length) {
    // Parse JSON command from client - use stack allocation
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<1024> doc;
    #pragma GCC diagnostic pop
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        LOG_W("Invalid WebSocket message from client %u", clientNum);
        return;
    }
    
    // Use shared command processor
    processCommand(doc);
}

/**
 * Processes a command received in JSON format.
 * 
 * This method is called from both handleWsMessage() (for local WebSocket commands)
 * and from the cloud connection command callback (for cloud-originated commands).
 * It handles commands from either source in a unified way.
 */
void WebServer::processCommand(JsonDocument& doc) {
    String type = doc["type"] | "";
    
    if (type == "ping") {
        _picoUart.sendPing();
    } 
    else if (type == "getConfig") {
        _picoUart.requestConfig();
    }
    else if (type == "setLogLevel") {
        // Set log level from WebSocket command
        // Expected payload: { type: "setLogLevel", level: "debug" | "info" | "warn" | "error" }
        String levelStr = doc["level"] | "info";
        BrewOSLogLevel level = stringToLogLevel(levelStr.c_str());
        setLogLevel(level);
        broadcastLog("Log level set to: %s", logLevelToString(level));
    }
    else if (type == "command") {
        // Handle commands from web UI
        String cmd = doc["cmd"] | "";
        
        if (cmd == "set_eco") {
            // Set eco mode configuration
            // Payload: enabled (bool), brewTemp (float), timeout (int minutes)
            bool enabled = doc["enabled"] | true;
            float brewTemp = doc["brewTemp"] | 80.0f;
            int timeout = doc["timeout"] | 30;
            
            // Convert to Pico format: [enabled:1][eco_brew_temp:2][timeout_minutes:2]
            uint8_t payload[5];
            payload[0] = enabled ? 1 : 0;
            int16_t tempScaled = (int16_t)(brewTemp * 10);  // Convert to Celsius * 10
            payload[1] = (tempScaled >> 8) & 0xFF;
            payload[2] = tempScaled & 0xFF;
            payload[3] = (timeout >> 8) & 0xFF;
            payload[4] = timeout & 0xFF;
            
            if (_picoUart.sendCommand(MSG_CMD_SET_ECO, payload, 5)) {
                broadcastLog("info", "Eco mode config sent: enabled=%s, temp=%.1f°C, timeout=%dmin", 
                         enabled ? "true" : "false", brewTemp, timeout);
            } else {
                broadcastLog("Failed to send eco config", "error");
            }
        }
        else if (cmd == "enter_eco") {
            // Manually enter eco mode
            uint8_t payload[1] = {1};  // 1 = enter eco
            if (_picoUart.sendCommand(MSG_CMD_SET_ECO, payload, 1)) {
                broadcastLog("Entering eco mode", "info");
            } else {
                broadcastLog("Failed to enter eco mode", "error");
            }
        }
        else if (cmd == "exit_eco") {
            // Manually exit eco mode (wake up)
            uint8_t payload[1] = {0};  // 0 = exit eco
            if (_picoUart.sendCommand(MSG_CMD_SET_ECO, payload, 1)) {
                broadcastLog("Exiting eco mode", "info");
            } else {
                broadcastLog("Failed to exit eco mode", "error");
            }
        }
        else if (cmd == "set_temp") {
            // Set temperature setpoint
            String boiler = doc["boiler"] | "brew";
            float temp = doc["temp"] | 0.0f;
            
            uint8_t payload[5];
            payload[0] = (boiler == "steam") ? 0x02 : 0x01;
            memcpy(&payload[1], &temp, sizeof(float));
            if (_picoUart.sendCommand(MSG_CMD_SET_TEMP, payload, 5)) {
                broadcastLog("%s temp set to %.1f°C", boiler.c_str(), temp);
            }
        }
        else if (cmd == "set_mode") {
            // Set machine mode
            String mode = doc["mode"] | "";
            uint8_t modeCmd = 0;
            
            // Check for optional heating strategy parameter
            if (!doc["strategy"].isNull() && mode == "on") {
                uint8_t strategy = doc["strategy"].as<uint8_t>();
                if (strategy <= 3) {  // Valid strategy range: 0-3
                    // Set heating strategy first
                    uint8_t strategyPayload[2] = {0x01, strategy};  // CONFIG_HEATING_STRATEGY = 0x01
                    _picoUart.sendCommand(MSG_CMD_CONFIG, strategyPayload, 2);
                    broadcastLog("Heating strategy set to: %d", strategy);
                }
            }
            
            if (mode == "on" || mode == "ready" || mode == "brew") {
                modeCmd = 0x01;  // MODE_BREW
            } else if (mode == "steam") {
                modeCmd = 0x02;  // MODE_STEAM
            } else if (mode == "off" || mode == "standby" || mode == "idle") {
                modeCmd = 0x00;  // MODE_IDLE
            } else if (mode == "eco") {
                // Enter eco mode
                uint8_t ecoPayload[1] = {1};
                _picoUart.sendCommand(MSG_CMD_SET_ECO, ecoPayload, 1);
                return;
            }
            
            if (_picoUart.sendCommand(MSG_CMD_MODE, &modeCmd, 1)) {
                broadcastLog("Mode set to: %s", mode.c_str());
            }
        }
        else if (cmd == "mqtt_test") {
            // Test MQTT connection
            MQTTConfig config = _mqttClient.getConfig();
            
            // Apply temporary config from command if provided
            if (!doc["broker"].isNull()) strncpy(config.broker, doc["broker"].as<const char*>(), sizeof(config.broker) - 1);
            if (!doc["port"].isNull()) config.port = doc["port"].as<uint16_t>();
            if (!doc["username"].isNull()) strncpy(config.username, doc["username"].as<const char*>(), sizeof(config.username) - 1);
            if (!doc["password"].isNull()) strncpy(config.password, doc["password"].as<const char*>(), sizeof(config.password) - 1);
            
            // Temporarily apply and test
            config.enabled = true;
            _mqttClient.setConfig(config);
            
            if (_mqttClient.testConnection()) {
                broadcastLog("MQTT connection test successful", "info");
            } else {
                broadcastLog("MQTT connection test failed", "error");
            }
        }
        else if (cmd == "mqtt_config") {
            // Update MQTT config
            MQTTConfig config = _mqttClient.getConfig();
            
            if (!doc["enabled"].isNull()) config.enabled = doc["enabled"].as<bool>();
            if (!doc["broker"].isNull()) strncpy(config.broker, doc["broker"].as<const char*>(), sizeof(config.broker) - 1);
            if (!doc["port"].isNull()) config.port = doc["port"].as<uint16_t>();
            if (!doc["username"].isNull()) strncpy(config.username, doc["username"].as<const char*>(), sizeof(config.username) - 1);
            if (!doc["password"].isNull()) {
                const char* pwd = doc["password"].as<const char*>();
                if (pwd && strlen(pwd) > 0) {
                    strncpy(config.password, pwd, sizeof(config.password) - 1);
                }
            }
            if (!doc["topic"].isNull()) {
                strncpy(config.topic_prefix, doc["topic"].as<const char*>(), sizeof(config.topic_prefix) - 1);
            }
            if (!doc["discovery"].isNull()) config.ha_discovery = doc["discovery"].as<bool>();
            
            if (_mqttClient.setConfig(config)) {
                broadcastLog("MQTT configuration updated", "info");
            }
        }
        else if (cmd == "set_cloud_config") {
            // Update cloud config
            auto& cloudSettings = State.settings().cloud;
            bool wasEnabled = cloudSettings.enabled;
            
            if (!doc["enabled"].isNull()) {
                cloudSettings.enabled = doc["enabled"].as<bool>();
            }
            if (!doc["serverUrl"].isNull()) {
                const char* url = doc["serverUrl"].as<const char*>();
                if (url) {
                    strncpy(cloudSettings.serverUrl, url, sizeof(cloudSettings.serverUrl) - 1);
                    cloudSettings.serverUrl[sizeof(cloudSettings.serverUrl) - 1] = '\0';
                }
            }
            
            // Save settings to NVS
            State.saveCloudSettings();
            
            // Update pairing manager based on enabled state
            if (_pairingManager) {
                if (cloudSettings.enabled && strlen(cloudSettings.serverUrl) > 0) {
                    // Initialize or update with new URL
                    _pairingManager->begin(String(cloudSettings.serverUrl));
                    broadcastLog("Cloud enabled: %s", cloudSettings.serverUrl);
                } else if (!cloudSettings.enabled && wasEnabled) {
                    // Cloud was disabled - clear pairing manager
                    _pairingManager->begin("");  // Clear cloud URL
                    broadcastLog("Cloud disabled", "info");
                }
            }
            
            broadcastLog("Cloud configuration updated: %s", cloudSettings.enabled ? "enabled" : "disabled");
        }
        else if (cmd == "add_schedule") {
            // Add a new schedule
            BrewOS::ScheduleEntry entry;
            entry.fromJson(doc.as<JsonObjectConst>());
            
            uint8_t newId = State.addSchedule(entry);
            if (newId > 0) {
                broadcastLog("Schedule added: %s", entry.name);
            }
        }
        else if (cmd == "update_schedule") {
            // Update existing schedule
            uint8_t id = doc["id"] | 0;
            if (id > 0) {
                BrewOS::ScheduleEntry entry;
                entry.fromJson(doc.as<JsonObjectConst>());
                if (State.updateSchedule(id, entry)) {
                    broadcastLog("Schedule updated", "info");
                }
            }
        }
        else if (cmd == "delete_schedule") {
            // Delete schedule
            uint8_t id = doc["id"] | 0;
            if (id > 0 && State.removeSchedule(id)) {
                broadcastLog("Schedule deleted", "info");
            }
        }
        else if (cmd == "toggle_schedule") {
            // Enable/disable schedule
            uint8_t id = doc["id"] | 0;
            bool enabled = doc["enabled"] | false;
            if (id > 0) {
                State.enableSchedule(id, enabled);
            }
        }
        else if (cmd == "set_auto_off") {
            // Set auto power-off settings
            bool enabled = doc["enabled"] | false;
            uint16_t minutes = doc["minutes"] | 60;
            State.setAutoPowerOff(enabled, minutes);
            broadcastLog("Auto power-off updated", "info");
        }
        else if (cmd == "get_schedules") {
            // Return schedules to requesting client (if needed)
            // Usually schedules are sent via full state broadcast
        }
        // Scale commands
        else if (cmd == "scale_scan") {
            if (!scaleManager || !scaleManager->isScanning()) {
                if (scaleManager && scaleManager->isConnected()) {
                    scaleManager->disconnect();
                }
                scaleManager->clearDiscovered();
                scaleManager->startScan(15000);
                broadcastLog("BLE scale scan started", "info");
            }
        }
        else if (cmd == "scale_scan_stop") {
            scaleManager->stopScan();
            broadcastLog("BLE scale scan stopped", "info");
        }
        else if (cmd == "scale_connect") {
            String address = doc["address"] | "";
            if (!address.isEmpty()) {
                scaleManager->connect(address.c_str());
                broadcastLog("Connecting to scale: %s", address.c_str());
            }
        }
        else if (cmd == "scale_disconnect") {
            scaleManager->disconnect();
            broadcastLog("Scale disconnected", "info");
        }
        else if (cmd == "tare" || cmd == "scale_tare") {
            scaleManager->tare();
            broadcastLog("Scale tared", "info");
        }
        else if (cmd == "scale_reset") {
            scaleManager->tare();
            brewByWeight->reset();
            broadcastLog("Scale reset", "info");
        }
        // Brew-by-weight settings
        else if (cmd == "set_bbw") {
            if (!doc["target_weight"].isNull()) {
                brewByWeight->setTargetWeight(doc["target_weight"].as<float>());
            }
            if (!doc["dose_weight"].isNull()) {
                brewByWeight->setDoseWeight(doc["dose_weight"].as<float>());
            }
            if (!doc["stop_offset"].isNull()) {
                brewByWeight->setStopOffset(doc["stop_offset"].as<float>());
            }
            if (!doc["auto_stop"].isNull()) {
                brewByWeight->setAutoStop(doc["auto_stop"].as<bool>());
            }
            if (!doc["auto_tare"].isNull()) {
                brewByWeight->setAutoTare(doc["auto_tare"].as<bool>());
            }
            broadcastLog("Brew-by-weight settings updated", "info");
        }
        // Pre-infusion settings
        else if (cmd == "set_preinfusion") {
            bool enabled = doc["enabled"] | false;
            uint16_t onTimeMs = doc["onTimeMs"] | 3000;
            uint16_t pauseTimeMs = doc["pauseTimeMs"] | 5000;
            
            // Validate timing parameters
            if (onTimeMs > 10000) {
                broadcastLog("Pre-infusion on_time too long (max 10000ms)", "error");
            } else if (pauseTimeMs > 30000) {
                broadcastLog("Pre-infusion pause_time too long (max 30000ms)", "error");
            } else {
                // Build payload for Pico: [config_type, enabled, on_time_ms(2), pause_time_ms(2)]
                uint8_t payload[6];
                payload[0] = CONFIG_PREINFUSION;  // Config type
                payload[1] = enabled ? 1 : 0;
                payload[2] = onTimeMs & 0xFF;         // Low byte
                payload[3] = (onTimeMs >> 8) & 0xFF;  // High byte
                payload[4] = pauseTimeMs & 0xFF;      // Low byte
                payload[5] = (pauseTimeMs >> 8) & 0xFF; // High byte
                
                if (_picoUart.sendCommand(MSG_CMD_CONFIG, payload, sizeof(payload))) {
                    // Update ESP32 state for persistence
                    auto& settings = State.settings();
                    settings.brew.preinfusionTime = onTimeMs / 1000.0f;  // Store as seconds
                    settings.brew.preinfusionPressure = enabled ? 1.0f : 0.0f;  // Use as enabled flag
                    State.saveBrewSettings();
                    
                    broadcastLog("info", "Pre-infusion settings saved: %s, on=%dms, pause=%dms", 
                             enabled ? "enabled" : "disabled", onTimeMs, pauseTimeMs);
                } else {
                    broadcastLog("Failed to send pre-infusion config to Pico", "error");
                }
            }
        }
        // Power settings
        else if (cmd == "set_power") {
            uint16_t voltage = doc["voltage"] | 230;
            uint8_t maxCurrent = doc["maxCurrent"] | 15;
            
            // Send to Pico as environmental config
            uint8_t payload[4];
            payload[0] = CONFIG_ENVIRONMENTAL;  // Config type
            payload[1] = (voltage >> 8) & 0xFF;
            payload[2] = voltage & 0xFF;
            payload[3] = maxCurrent;
            _picoUart.sendCommand(MSG_CMD_CONFIG, payload, 4);
            
            // Also save to local settings
            State.settings().power.mainsVoltage = voltage;
            State.settings().power.maxCurrent = (float)maxCurrent;
            State.savePowerSettings();
            
            broadcastLog("Power settings updated: %dV, %.1fA", voltage, maxCurrent);
        }
        // Power meter configuration
        else if (cmd == "configure_power_meter") {
            String source = doc["source"] | "none";
            
            if (source == "none") {
                powerMeterManager->setSource(PowerMeterSource::NONE);
                // Send disable command to Pico
                uint8_t payload[1] = {0};  // 0 = disabled
                _picoUart.sendCommand(MSG_CMD_POWER_METER_CONFIG, payload, 1);
                broadcastLog("Power metering disabled", "info");
            }
            else if (source == "hardware") {
                // Hardware meters are configured on Pico
                powerMeterManager->configureHardware();
                
                // Send enable command to Pico
                uint8_t payload[1] = {1};  // 1 = enabled
                _picoUart.sendCommand(MSG_CMD_POWER_METER_CONFIG, payload, 1);
                broadcastLog("Power meter configured (hardware)", "info");
            }
            else if (source == "mqtt") {
                String topic = doc["topic"] | "";
                String format = doc["format"] | "auto";
                
                if (topic.length() > 0) {
                    if (powerMeterManager && powerMeterManager->configureMqtt(topic.c_str(), format.c_str())) {
                        broadcastLog("MQTT power meter configured: %s", topic.c_str());
                    } else {
                        broadcastLog("Failed to configure MQTT power meter", "error");
                    }
                } else {
                    broadcastLog("MQTT topic required", "error");
                }
            }
            
            // Broadcast updated status
            broadcastPowerMeterStatus();
        }
        else if (cmd == "start_power_meter_discovery") {
            // Forward discovery command to Pico
            _picoUart.sendCommand(MSG_CMD_POWER_METER_DISCOVER, nullptr, 0);
            powerMeterManager->startAutoDiscovery();
            broadcastLog("Starting power meter auto-discovery...", "info");
        }
        // WiFi commands
        else if (cmd == "wifi_forget") {
            _wifiManager.clearCredentials();
            broadcastLog("WiFi credentials cleared. Device will restart.", "warn");
            delay(1000);
            ESP.restart();
        }
        else if (cmd == "wifi_config") {
            // Static IP configuration
            bool staticIp = doc["staticIp"] | false;
            String ip = doc["ip"] | "";
            String gateway = doc["gateway"] | "";
            String subnet = doc["subnet"] | "255.255.255.0";
            String dns1 = doc["dns1"] | "";
            String dns2 = doc["dns2"] | "";
            
            _wifiManager.setStaticIP(staticIp, ip, gateway, subnet, dns1, dns2);
            
            if (staticIp) {
                broadcastLog("Static IP configured: %s. Reconnecting...", ip.c_str());
            } else {
                broadcastLog("DHCP mode enabled. Reconnecting...", "info");
            }
            
            // Reconnect to apply new settings
            delay(500);
            _wifiManager.connectToWiFi();
        }
        // System commands
        else if (cmd == "restart") {
            broadcastLog("Device restarting...", "warn");
            delay(500);
            ESP.restart();
        }
        else if (cmd == "factory_reset") {
            broadcastLog("Factory reset initiated...", "warn");
            State.factoryReset();
            _wifiManager.clearCredentials();
            delay(1000);
            ESP.restart();
        }
        else if (cmd == "check_update") {
            // Check for OTA updates from GitHub releases
            checkForUpdates();
        }
        else if (cmd == "ota_start") {
            // Update BrewOS firmware (combined: Pico first, then ESP32)
            String version = doc["version"] | "";
            if (version.isEmpty()) {
                broadcastLog("OTA error: No version specified", "error");
            } else {
                startCombinedOTA(version);
            }
        }
        else if (cmd == "esp32_ota_start") {
            // ESP32 only - for recovery/advanced use
            String version = doc["version"] | "";
            if (version.isEmpty()) {
                broadcastLog("OTA error: No version specified", "error");
            } else {
                startGitHubOTA(version);
            }
        }
        else if (cmd == "pico_ota_start") {
            // Pico only - for recovery/advanced use
            String version = doc["version"] | "";
            if (version.isEmpty()) {
                broadcastLog("OTA error: No version specified", "error");
            } else {
                startPicoGitHubOTA(version);
            }
        }
        else if (cmd == "check_version_mismatch") {
            // Check and report firmware version mismatch
            checkVersionMismatch();
        }
        // Machine info
        else if (cmd == "set_machine_info" || cmd == "set_device_info") {
            auto& machineInfo = State.settings().machineInfo;
            auto& networkSettings = State.settings().network;
            
            if (!doc["name"].isNull()) {
                strncpy(machineInfo.deviceName, doc["name"].as<const char*>(), sizeof(machineInfo.deviceName) - 1);
                machineInfo.deviceName[sizeof(machineInfo.deviceName) - 1] = '\0';
                // Also update hostname for mDNS
                strncpy(networkSettings.hostname, doc["name"].as<const char*>(), sizeof(networkSettings.hostname) - 1);
                networkSettings.hostname[sizeof(networkSettings.hostname) - 1] = '\0';
            }
            if (!doc["brand"].isNull()) {
                strncpy(machineInfo.machineBrand, doc["brand"].as<const char*>(), sizeof(machineInfo.machineBrand) - 1);
                machineInfo.machineBrand[sizeof(machineInfo.machineBrand) - 1] = '\0';
            }
            if (!doc["model"].isNull()) {
                strncpy(machineInfo.machineModel, doc["model"].as<const char*>(), sizeof(machineInfo.machineModel) - 1);
                machineInfo.machineModel[sizeof(machineInfo.machineModel) - 1] = '\0';
            }
            if (!doc["machineType"].isNull()) {
                strncpy(machineInfo.machineType, doc["machineType"].as<const char*>(), sizeof(machineInfo.machineType) - 1);
                machineInfo.machineType[sizeof(machineInfo.machineType) - 1] = '\0';
            }
            
            State.saveMachineInfoSettings();
            State.saveNetworkSettings();
            
            // Broadcast device info update
            broadcastDeviceInfo();
            broadcastLog("Device info updated: %s", machineInfo.deviceName);
        }
        // User preferences (synced across devices)
        else if (cmd == "set_preferences") {
            auto& prefs = State.settings().preferences;
            
            if (!doc["firstDayOfWeek"].isNull()) {
                const char* dow = doc["firstDayOfWeek"];
                prefs.firstDayOfWeek = (strcmp(dow, "monday") == 0) ? 1 : 0;
            }
            if (!doc["use24HourTime"].isNull()) {
                prefs.use24HourTime = doc["use24HourTime"];
            }
            if (!doc["temperatureUnit"].isNull()) {
                const char* unit = doc["temperatureUnit"];
                prefs.temperatureUnit = (strcmp(unit, "fahrenheit") == 0) ? 1 : 0;
            }
            if (!doc["electricityPrice"].isNull()) {
                prefs.electricityPrice = doc["electricityPrice"];
            }
            if (!doc["currency"].isNull()) {
                strncpy(prefs.currency, doc["currency"].as<const char*>(), sizeof(prefs.currency) - 1);
                prefs.currency[sizeof(prefs.currency) - 1] = '\0';
            }
            if (!doc["lastHeatingStrategy"].isNull()) {
                prefs.lastHeatingStrategy = doc["lastHeatingStrategy"];
            }
            
            // Mark as initialized once browser sends first preferences
            prefs.initialized = true;
            
            State.saveUserPreferences();
            broadcastDeviceInfo();
            broadcastLog("User preferences updated", "info");
        }
        // Maintenance records
        else if (cmd == "record_maintenance") {
            String type = doc["type"] | "";
            if (!type.isEmpty()) {
                State.recordMaintenance(type.c_str());
                broadcastLog("Maintenance recorded: %s", type.c_str());
            }
        }
        // Diagnostics commands
        else if (cmd == "run_diagnostics") {
            // Run all diagnostic tests
            uint8_t payload[1] = { 0x00 };  // DIAG_TEST_ALL
            if (_picoUart.sendCommand(MSG_CMD_DIAGNOSTICS, payload, 1)) {
                broadcastLog("Running hardware diagnostics...", "info");
            } else {
                broadcastLog("Failed to start diagnostics", "error");
            }
        }
        else if (cmd == "run_diagnostic_test") {
            // Run a single diagnostic test
            uint8_t testId = doc["testId"] | 0;
            uint8_t payload[1] = { testId };
            if (_picoUart.sendCommand(MSG_CMD_DIAGNOSTICS, payload, 1)) {
                broadcastLog("Running diagnostic test %d", testId);
            } else {
                broadcastLog("Failed to start diagnostic test", "error");
            }
        }
    }
}

// Internal helper to broadcast a formatted log message
static void broadcastLogInternal(AsyncWebSocket* ws, CloudConnection* cloudConnection, 
                                 const char* level, const char* message) {
    if (!message || !ws) return;
    
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<512> doc;
    #pragma GCC diagnostic pop
    doc["type"] = "log";
    doc["level"] = level ? level : "info";
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    // Allocate JSON buffer in internal RAM (not PSRAM) to avoid crashes
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        // Fallback to regular malloc if heap_caps_malloc fails
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        ws->textAll(jsonBuffer);
        
        // Also send to cloud - use jsonBuffer directly to avoid String allocation
        if (cloudConnection) {
            cloudConnection->send(jsonBuffer);
        }
        
        free(jsonBuffer);
    }
}

// Variadic version - format string with arguments (like printf)
// Variadic version - format string with arguments (like printf), defaults to "info"
void WebServer::broadcastLog(const char* format, ...) {
    if (!format) return;
    
    // Format message into stack-allocated buffer (internal RAM, not PSRAM)
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    broadcastLogInternal(&_ws, _cloudConnection, "info", message);
}

// Variadic version with explicit level (format, level, ...args)
void WebServer::broadcastLog(const char* format, const char* level, ...) {
    if (!format || !level) return;
    
    // Format message into stack-allocated buffer (internal RAM, not PSRAM)
    char message[256];
    va_list args;
    va_start(args, level);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    broadcastLogInternal(&_ws, _cloudConnection, level, message);
}


void WebServer::broadcastPicoMessage(uint8_t type, const uint8_t* payload, size_t len) {
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<512> doc;
    #pragma GCC diagnostic pop
    doc["type"] = "pico";
    doc["msgType"] = type;
    doc["timestamp"] = millis();
    
    // Convert payload to hex string for debugging - use stack buffer
    char hexPayload[128] = {0};  // Max 56 bytes * 2 = 112 chars + null
    size_t hexLen = 0;
    for (size_t i = 0; i < len && i < 56 && hexLen < sizeof(hexPayload) - 2; i++) {
        hexLen += snprintf(hexPayload + hexLen, sizeof(hexPayload) - hexLen, "%02X", payload[i]);
    }
    doc["payload"] = hexPayload;
    doc["length"] = len;
    
    // Allocate JSON buffer in internal RAM (not PSRAM) to avoid crashes
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        _ws.textAll(jsonBuffer);
        
        // Also send to cloud - use jsonBuffer directly to avoid String allocation
        if (_cloudConnection) {
            _cloudConnection->send(jsonBuffer);
        }
        
        free(jsonBuffer);
    }
}

void WebServer::broadcastRaw(const String& json) {
    _ws.textAll(json.c_str(), json.length());
    
    // Also send to cloud
    if (_cloudConnection) {
        _cloudConnection->send(json);
    }
}

void WebServer::broadcastRaw(const char* json) {
    if (!json) return;
    _ws.textAll(json);
    
    // Also send to cloud
    if (_cloudConnection) {
        _cloudConnection->send(json);
    }
}

// =============================================================================
// Unified Status Broadcast - Single comprehensive message
// =============================================================================
void WebServer::broadcastFullStatus(const ui_state_t& state) {
    // Safety check - only broadcast if we have clients
    if (_ws.count() == 0 && (!_cloudConnection || !_cloudConnection->isConnected())) {
        return;  // No one to send to
    }
    
    // Use heap allocation to avoid stack overflow (4KB is too large for stack)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    DynamicJsonDocument* docPtr = new DynamicJsonDocument(4096);
    if (!docPtr) {
        LOG_E("Failed to allocate JSON document for status broadcast");
        return;
    }
    DynamicJsonDocument& doc = *docPtr;
    #pragma GCC diagnostic pop
    doc["type"] = "status";
    
    // Timestamps - track machine on time and last shot
    // Use Unix timestamps (milliseconds) for client compatibility
    static uint64_t machineOnTimestamp = 0;
    static uint64_t lastShotTimestamp = 0;
    static bool wasOn = false;
    
    // Machine is "on" when in active states (heating through cooldown)
    bool isOn = state.machine_state >= UI_STATE_HEATING && state.machine_state <= UI_STATE_COOLDOWN;
    
    // Track when machine turns on (Unix timestamp in milliseconds)
    // Bounds: year 2020 (1577836800) to year 2100 (4102444800) for overflow protection
    constexpr time_t MIN_VALID_TIME = 1577836800;  // Jan 1, 2020
    constexpr time_t MAX_VALID_TIME = 4102444800;  // Jan 1, 2100
    
    if (isOn && !wasOn) {
        time_t now = time(nullptr);
        // Only set timestamp if NTP is synced and within reasonable bounds
        if (now > MIN_VALID_TIME && now < MAX_VALID_TIME) {
            machineOnTimestamp = (uint64_t)now * 1000ULL;
        } else {
            machineOnTimestamp = 0;  // Time not synced or out of bounds
        }
    } else if (!isOn) {
        machineOnTimestamp = 0;
    }
    wasOn = isOn;
    
    // Track last shot timestamp (Unix timestamp in milliseconds)
    static bool wasBrewing = false;
    if (wasBrewing && !state.is_brewing) {
        time_t now = time(nullptr);
        // Only set timestamp if NTP is synced and within reasonable bounds
        if (now > MIN_VALID_TIME && now < MAX_VALID_TIME) {
            lastShotTimestamp = (uint64_t)now * 1000ULL;
        }
    }
    wasBrewing = state.is_brewing;
    
    // =========================================================================
    // Machine Section
    // =========================================================================
    JsonObject machine = doc["machine"].to<JsonObject>();
    
    // Machine state - convert to string for web client
    const char* stateStr = "unknown";
    switch (state.machine_state) {
        case UI_STATE_INIT: stateStr = "init"; break;
        case UI_STATE_IDLE: stateStr = "idle"; break;
        case UI_STATE_HEATING: stateStr = "heating"; break;
        case UI_STATE_READY: stateStr = "ready"; break;
        case UI_STATE_BREWING: stateStr = "brewing"; break;
        case UI_STATE_STEAMING: stateStr = "steaming"; break;
        case UI_STATE_COOLDOWN: stateStr = "cooldown"; break;
        case UI_STATE_FAULT: stateStr = "fault"; break;
        case UI_STATE_SAFE: stateStr = "safe"; break;
    }
    machine["state"] = stateStr;
    
    // Machine mode - derive from state
    const char* modeStr = "standby";
    if (state.machine_state >= UI_STATE_HEATING && state.machine_state <= UI_STATE_COOLDOWN) {
        modeStr = "on";
    }
    machine["mode"] = modeStr;
    machine["isHeating"] = state.is_heating;
    machine["isBrewing"] = state.is_brewing;
    machine["heatingStrategy"] = state.heating_strategy;
    
    // Timestamps (Unix milliseconds for JavaScript compatibility)
    if (machineOnTimestamp > 0) {
        machine["machineOnTimestamp"] = (double)machineOnTimestamp;  // Cast to double for JSON
    } else {
        machine["machineOnTimestamp"] = (char*)nullptr;
    }
    if (lastShotTimestamp > 0) {
        machine["lastShotTimestamp"] = (double)lastShotTimestamp;  // Cast to double for JSON
    } else {
        machine["lastShotTimestamp"] = (char*)nullptr;
    }
    
    // =========================================================================
    // Temperatures Section
    // =========================================================================
    JsonObject temps = doc["temps"].to<JsonObject>();
    JsonObject brew = temps["brew"].to<JsonObject>();
    brew["current"] = state.brew_temp;
    brew["setpoint"] = state.brew_setpoint;
    
    JsonObject steam = temps["steam"].to<JsonObject>();
    steam["current"] = state.steam_temp;
    steam["setpoint"] = state.steam_setpoint;
    
    temps["group"] = state.group_temp;
    
    // =========================================================================
    // Pressure
    // =========================================================================
    doc["pressure"] = state.pressure;
    
    // =========================================================================
    // Power Section
    // =========================================================================
    JsonObject power = doc["power"].to<JsonObject>();
    power["current"] = state.power_watts;
    
    // Get power meter reading if available
    PowerMeterReading meterReading;
    if (powerMeterManager && powerMeterManager->getReading(meterReading)) {
        power["voltage"] = meterReading.voltage;
        power["todayKwh"] = powerMeterManager->getTodayKwh();  // Daily energy (since midnight)
        power["totalKwh"] = powerMeterManager->getTotalKwh();  // Total energy from meter
        
        // Add meter info
        JsonObject meter = power["meter"].to<JsonObject>();
        meter["source"] = powerMeterManager ? powerMeterSourceToString(powerMeterManager->getSource()) : "none";
        meter["connected"] = powerMeterManager ? powerMeterManager->isConnected() : false;
        meter["meterType"] = powerMeterManager ? powerMeterManager->getMeterName() : "";
        meter["lastUpdate"] = meterReading.timestamp;
        
        JsonObject reading = meter["reading"].to<JsonObject>();
        reading["voltage"] = meterReading.voltage;
        reading["current"] = meterReading.current;
        reading["power"] = meterReading.power;
        reading["energy"] = meterReading.energy_import;
        reading["frequency"] = meterReading.frequency;
        reading["powerFactor"] = meterReading.power_factor;
    } else {
        // Fallback to configured voltage
        power["voltage"] = State.settings().power.mainsVoltage;
        power["todayKwh"] = 0;
        power["totalKwh"] = 0;
    }
    
    power["maxCurrent"] = State.settings().power.maxCurrent;
    
    // =========================================================================
    // Stats Section - Key metrics for dashboard
    // =========================================================================
    JsonObject stats = doc["stats"].to<JsonObject>();
    
    // Get current statistics
    BrewOS::FullStatistics fullStats;
    Stats.getFullStatistics(fullStats);
    
    // Daily stats
    JsonObject daily = stats["daily"].to<JsonObject>();
    BrewOS::PeriodStats dailyStats;
    Stats.getDailyStats(dailyStats);
    daily["shotCount"] = dailyStats.shotCount;
    daily["avgBrewTimeMs"] = dailyStats.avgBrewTimeMs;
    daily["totalKwh"] = dailyStats.totalKwh;
    
    // Lifetime stats
    JsonObject lifetime = stats["lifetime"].to<JsonObject>();
    lifetime["totalShots"] = fullStats.lifetime.totalShots;
    lifetime["avgBrewTimeMs"] = fullStats.lifetime.avgBrewTimeMs;
    lifetime["totalKwh"] = fullStats.lifetime.totalKwh;
    
    // Session stats
    stats["sessionShots"] = fullStats.sessionShots;
    stats["shotsToday"] = dailyStats.shotCount;
    
    // =========================================================================
    // Cleaning Section
    // =========================================================================
    JsonObject cleaning = doc["cleaning"].to<JsonObject>();
    cleaning["brewCount"] = state.brew_count;
    cleaning["reminderDue"] = state.cleaning_reminder;
    
    // =========================================================================
    // Water Section
    // =========================================================================
    JsonObject water = doc["water"].to<JsonObject>();
    water["tankLevel"] = state.water_low ? "low" : "ok";
    
    // =========================================================================
    // Scale Section
    // =========================================================================
    JsonObject scale = doc["scale"].to<JsonObject>();
    scale["connected"] = state.scale_connected;
    scale["weight"] = state.brew_weight;
    scale["flowRate"] = state.flow_rate;
    // Scale name and type come from scaleManager (accessed in main.cpp)
    
    // =========================================================================
    // Connections Section
    // =========================================================================
    JsonObject connections = doc["connections"].to<JsonObject>();
    connections["pico"] = state.pico_connected;
    connections["wifi"] = state.wifi_connected;
    connections["mqtt"] = state.mqtt_connected;
    connections["scale"] = state.scale_connected;
    connections["cloud"] = state.cloud_connected;
    
    // =========================================================================
    // ESP32 Info
    // =========================================================================
    JsonObject esp32 = doc["esp32"].to<JsonObject>();
    esp32["version"] = ESP32_VERSION;
    esp32["freeHeap"] = ESP.getFreeHeap();
    esp32["uptime"] = millis();
    
    // Allocate JSON buffer in internal RAM (not PSRAM) to avoid crashes
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        // Fallback to regular malloc if heap_caps_malloc fails
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        
        // Send to WebSocket clients (check count again to be safe)
        if (_ws.count() > 0) {
            _ws.textAll(jsonBuffer);
        }
        
        // Also send to cloud - use jsonBuffer directly to avoid String allocation
        if (_cloudConnection && _cloudConnection->isConnected()) {
            _cloudConnection->send(jsonBuffer);
        }
        
        free(jsonBuffer);
    }
    
    // Free the JSON document
    delete docPtr;
}

void WebServer::broadcastDeviceInfo() {
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<2048> doc;
    #pragma GCC diagnostic pop
    doc["type"] = "device_info";
    
    // Get device info from state manager
    const auto& machineInfo = State.settings().machineInfo;
    const auto& cloudSettings = State.settings().cloud;
    const auto& powerSettings = State.settings().power;
    const auto& tempSettings = State.settings().temperature;
    const auto& prefs = State.settings().preferences;
    
    doc["deviceId"] = cloudSettings.deviceId;
    doc["deviceName"] = machineInfo.deviceName;
    doc["machineBrand"] = machineInfo.machineBrand;
    doc["machineModel"] = machineInfo.machineModel;
    doc["machineType"] = machineInfo.machineType;
    doc["firmwareVersion"] = ESP32_VERSION;
    
    // Include power settings
    doc["mainsVoltage"] = powerSettings.mainsVoltage;
    doc["maxCurrent"] = powerSettings.maxCurrent;
    
    // Include eco mode settings
    doc["ecoBrewTemp"] = tempSettings.ecoBrewTemp;
    doc["ecoTimeoutMinutes"] = tempSettings.ecoTimeoutMinutes;
    
    // Include pre-infusion settings
    const auto& brewSettings = State.settings().brew;
    // preinfusionPressure > 0 is used as the enabled flag (legacy compatibility)
    doc["preinfusionEnabled"] = brewSettings.preinfusionPressure > 0;
    doc["preinfusionOnMs"] = (uint16_t)(brewSettings.preinfusionTime * 1000);  // Convert seconds to ms
    doc["preinfusionPauseMs"] = (uint16_t)(brewSettings.preinfusionPressure > 0 ? 5000 : 0);  // Default pause
    
    // Include user preferences (synced across devices)
    JsonObject preferences = doc["preferences"].to<JsonObject>();
    prefs.toJson(preferences);
    
    // Allocate JSON buffer in internal RAM (not PSRAM) to avoid crashes
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        _ws.textAll(jsonBuffer);
        
        // Also send to cloud - use jsonBuffer directly to avoid String allocation
        if (_cloudConnection) {
            _cloudConnection->send(jsonBuffer);
        }
        
        free(jsonBuffer);
    }
}

void WebServer::broadcastPowerMeterStatus() {
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<1024> doc;
    #pragma GCC diagnostic pop
    doc["type"] = "power_meter_status";
    
    // Get status from power meter manager
    if (powerMeterManager) powerMeterManager->getStatus(doc);
    
    // Allocate JSON buffer in internal RAM (not PSRAM) to avoid crashes
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        _ws.textAll(jsonBuffer);
        
        // Also send to cloud - use jsonBuffer directly to avoid String allocation
        if (_cloudConnection) {
            _cloudConnection->send(jsonBuffer);
        }
        
        free(jsonBuffer);
    }
}

void WebServer::broadcastEvent(const String& event, const JsonDocument* data) {
    // Use stack allocation to avoid PSRAM crashes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<1024> doc;
    #pragma GCC diagnostic pop
    doc["type"] = "event";
    // Copy event string to avoid PSRAM pointer issues
    char eventBuf[64];
    strncpy(eventBuf, event.c_str(), sizeof(eventBuf) - 1);
    eventBuf[sizeof(eventBuf) - 1] = '\0';
    doc["event"] = eventBuf;
    doc["timestamp"] = millis();
    
    if (data) {
        doc["data"] = *data;
    }
    
    // Allocate JSON buffer in internal RAM (not PSRAM) to avoid crashes
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) {
        jsonBuffer = (char*)malloc(jsonSize);
    }
    
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        _ws.textAll(jsonBuffer);
        
        // Also send to cloud - use jsonBuffer directly to avoid String allocation
        if (_cloudConnection) {
            _cloudConnection->send(jsonBuffer);
        }
        
        free(jsonBuffer);
    }
}

size_t WebServer::getClientCount() {
    return _ws.count();
}

String WebServer::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";
    if (filename.endsWith(".png")) return "image/png";
    if (filename.endsWith(".ico")) return "image/x-icon";
    return "text/plain";
}

bool WebServer::streamFirmwareToPico(File& firmwareFile, size_t firmwareSize) {
    const size_t CHUNK_SIZE = 200;  // Bootloader protocol supports up to 256 bytes per chunk
    uint8_t buffer[CHUNK_SIZE];
    size_t bytesSent = 0;
    uint32_t chunkNumber = 0;
    
    firmwareFile.seek(0);  // Start from beginning
    
    // Stream firmware in chunks using bootloader protocol
    while (bytesSent < firmwareSize) {
        size_t bytesToRead = min(CHUNK_SIZE, firmwareSize - bytesSent);
        size_t bytesRead = firmwareFile.read(buffer, bytesToRead);
        
        if (bytesRead == 0) {
            LOG_E("Failed to read firmware chunk at offset %d", bytesSent);
            broadcastLog("Firmware read error", "error");
            return false;
        }
        
        // Stream chunk via bootloader protocol (raw UART, not packet protocol)
        size_t sent = _picoUart.streamFirmwareChunk(buffer, bytesRead, chunkNumber);
        if (sent != bytesRead) {
            LOG_E("Failed to send chunk %d: %d/%d bytes", chunkNumber, sent, bytesRead);
            broadcastLog("Firmware streaming error at chunk %d", "error", chunkNumber);
            return false;
        }
        
        bytesSent += bytesRead;
        chunkNumber++;
        
        // Report progress every 10%
        static size_t lastProgress = 0;
        size_t progress = (bytesSent * 100) / firmwareSize;
        if (progress >= lastProgress + 10 || bytesSent == firmwareSize) {
            lastProgress = progress;
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            StaticJsonDocument<256> doc;
            #pragma GCC diagnostic pop
            doc["type"] = "ota_progress";
            doc["stage"] = "flash";
            doc["progress"] = progress;
            doc["sent"] = bytesSent;
            doc["total"] = firmwareSize;
            
            size_t jsonSize = measureJson(doc) + 1;
            char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
            if (jsonBuffer) {
                serializeJson(doc, jsonBuffer, jsonSize);
                _ws.textAll(jsonBuffer);
                free(jsonBuffer);
            }
            LOG_I("Flash progress: %d%% (%d/%d bytes)", progress, bytesSent, firmwareSize);
        }
        
        // Small delay to prevent UART buffer overflow
        delay(10);
    }
    
    // Send end marker (chunk number 0xFFFFFFFF signals end of firmware)
    uint8_t endMarker[2] = {0xAA, 0x55};  // Bootloader end magic
    size_t sent = _picoUart.streamFirmwareChunk(endMarker, 2, 0xFFFFFFFF);
    if (sent != 2) {
        LOG_E("Failed to send end marker");
        broadcastLog("Failed to send end marker", "error");
        return false;
    }
    
    LOG_I("Firmware streaming complete: %d bytes in %d chunks", bytesSent, chunkNumber);
    broadcastLog("Firmware streaming complete: %zu bytes in %d chunks", bytesSent, chunkNumber);
    return true;
}

void WebServer::handleGetMQTTConfig(AsyncWebServerRequest* request) {
    MQTTConfig config = _mqttClient.getConfig();
    
    #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<2048> doc;
        #pragma GCC diagnostic pop
    doc["enabled"] = config.enabled;
    doc["broker"] = config.broker;
    doc["port"] = config.port;
    doc["username"] = config.username;
    doc["password"] = "";  // Don't send password back
    doc["client_id"] = config.client_id;
    doc["topic_prefix"] = config.topic_prefix;
    doc["use_tls"] = config.use_tls;
    doc["ha_discovery"] = config.ha_discovery;
    doc["ha_device_id"] = config.ha_device_id;
    doc["connected"] = _mqttClient.isConnected();
    // Get MQTT status - copy String to stack buffer to avoid PSRAM
    String mqttStatusStr = _mqttClient.getStatusString();
    char mqttStatusBuf[32];
    strncpy(mqttStatusBuf, mqttStatusStr.c_str(), sizeof(mqttStatusBuf) - 1);
    mqttStatusBuf[sizeof(mqttStatusBuf) - 1] = '\0';
    doc["status"] = mqttStatusBuf;
    
    
        // Allocate JSON buffer in internal RAM (not PSRAM)
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) {
            jsonBuffer = (char*)malloc(jsonSize);
        }
        
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            request->send(200, "application/json", jsonBuffer);
            // Buffer will be freed by AsyncWebServer after response
        } else {
            request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        }
}

void WebServer::handleSetMQTTConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    MQTTConfig config = _mqttClient.getConfig();
    
    // Update from JSON - use is<T>() to check if key exists (ArduinoJson 7.x API)
    if (!doc["enabled"].isNull()) config.enabled = doc["enabled"].as<bool>();
    if (!doc["broker"].isNull()) strncpy(config.broker, doc["broker"].as<const char*>(), sizeof(config.broker) - 1);
    if (!doc["port"].isNull()) config.port = doc["port"].as<uint16_t>();
    if (!doc["username"].isNull()) strncpy(config.username, doc["username"].as<const char*>(), sizeof(config.username) - 1);
    
    // Only update password if provided and non-empty
    if (!doc["password"].isNull()) {
        const char* pwd = doc["password"].as<const char*>();
        if (pwd && strlen(pwd) > 0) {
            strncpy(config.password, pwd, sizeof(config.password) - 1);
        }
    }
    
    if (!doc["client_id"].isNull()) strncpy(config.client_id, doc["client_id"].as<const char*>(), sizeof(config.client_id) - 1);
    if (!doc["topic_prefix"].isNull()) {
        const char* prefix = doc["topic_prefix"].as<const char*>();
        if (prefix && strlen(prefix) > 0) {
            strncpy(config.topic_prefix, prefix, sizeof(config.topic_prefix) - 1);
        }
    }
    if (!doc["use_tls"].isNull()) config.use_tls = doc["use_tls"].as<bool>();
    if (!doc["ha_discovery"].isNull()) config.ha_discovery = doc["ha_discovery"].as<bool>();
    if (!doc["ha_device_id"].isNull()) strncpy(config.ha_device_id, doc["ha_device_id"].as<const char*>(), sizeof(config.ha_device_id) - 1);
    
    if (_mqttClient.setConfig(config)) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
        broadcastLog("MQTT configuration updated", "info");
    } else {
        request->send(400, "application/json", "{\"error\":\"Invalid configuration\"}");
    }
}

void WebServer::handleTestMQTT(AsyncWebServerRequest* request) {
    if (_mqttClient.testConnection()) {
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Connection successful\"}");
        broadcastLog("MQTT connection test successful", "info");
    } else {
        request->send(500, "application/json", "{\"error\":\"Connection failed\"}");
        broadcastLog("MQTT connection test failed", "error");
    }
}

// =============================================================================
// GitHub OTA - Download and install ESP32 firmware from GitHub releases
// =============================================================================

// Helper to broadcast OTA progress - uses stack allocation
static void broadcastOtaProgress(AsyncWebSocket* ws, const char* stage, int progress, const char* message) {
    if (!ws) return;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    StaticJsonDocument<256> doc;
    #pragma GCC diagnostic pop
    doc["type"] = "ota_progress";
    doc["stage"] = stage;
    doc["progress"] = progress;
    doc["message"] = message;
    
    size_t jsonSize = measureJson(doc) + 1;
    char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
    if (jsonBuffer) {
        serializeJson(doc, jsonBuffer, jsonSize);
        ws->textAll(jsonBuffer);
        free(jsonBuffer);
    }
}

void WebServer::startGitHubOTA(const String& version) {
    LOG_I("Starting ESP32 GitHub OTA for version: %s", version.c_str());
    
    // Define macro for consistent OTA progress broadcasting
    #define broadcastProgress(stage, progress, message) broadcastOtaProgress(&_ws, stage, progress, message)
    
    broadcastProgress("flash", 65, "Completing update...");
    
    // Build the GitHub release download URL
    // Build download URL using stack buffer to avoid PSRAM
    char tag[32];
    if (strcmp(version.c_str(), "dev-latest") != 0 && strncmp(version.c_str(), "v", 1) != 0) {
        snprintf(tag, sizeof(tag), "v%s", version.c_str());
    } else {
        strncpy(tag, version.c_str(), sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';
    }
    
    char downloadUrl[256];
    snprintf(downloadUrl, sizeof(downloadUrl), 
             "https://github.com/" GITHUB_OWNER "/" GITHUB_REPO "/releases/download/%s/" GITHUB_ESP32_ASSET, 
             tag);
    LOG_I("Download URL: %s", downloadUrl);
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(30000);
    
    if (!http.begin(downloadUrl)) {
        LOG_E("Failed to connect to GitHub");
        broadcastLog("Update error: Cannot connect to server", "error");
        broadcastOtaProgress(&_ws, "error", 0, "Connection failed");
        return;
    }
    
    http.addHeader("User-Agent", "BrewOS-ESP32/" ESP32_VERSION);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        LOG_E("HTTP error: %d", httpCode);
        const char* errorMsg = (httpCode == 404) ? "Update not found" : "Download failed";
        broadcastLog("Update error: %s", "error", errorMsg);
        broadcastOtaProgress(&_ws, "error", 0, errorMsg);
        http.end();
        return;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        LOG_E("Invalid content length: %d", contentLength);
        broadcastLog("Update error: Invalid firmware", "error");
        broadcastOtaProgress(&_ws, "error", 0, "Invalid firmware");
        http.end();
        return;
    }
    
    LOG_I("ESP32 firmware size: %d bytes", contentLength);
    broadcastOtaProgress(&_ws, "download", 70, "Downloading...");
    
    // Begin OTA update
    if (!Update.begin(contentLength)) {
        LOG_E("Not enough space for OTA");
        broadcastLog("Update error: Not enough space", "error");
        broadcastOtaProgress(&_ws, "error", 0, "Not enough space");
        http.end();
        return;
    }
    
    // Stream the firmware to Update
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    size_t written = 0;
    int lastProgress = 0;
    
    while (http.connected() && written < (size_t)contentLength) {
        size_t available = stream->available();
        if (available > 0) {
            size_t toRead = min(available, sizeof(buffer));
            size_t bytesRead = stream->readBytes(buffer, toRead);
            
            if (bytesRead > 0) {
                size_t bytesWritten = Update.write(buffer, bytesRead);
                if (bytesWritten != bytesRead) {
                    LOG_E("Write error at offset %d", written);
                    broadcastLog("Update error: Write failed", "error");
                    broadcastProgress("error", 0, "Write failed");
                    Update.abort();
                    http.end();
                    return;
                }
                written += bytesWritten;
                
                // Report progress 70-95%
                int progress = 70 + (written * 25) / contentLength;
                if (progress >= lastProgress + 5) {
                    lastProgress = progress;
                    LOG_I("OTA progress: %d%% (%d/%d)", progress, written, contentLength);
                    broadcastProgress("download", progress, "Installing...");
                }
            }
        }
        delay(1);  // Yield to other tasks
    }
    
    http.end();
    
    if (written != (size_t)contentLength) {
        LOG_E("Download incomplete: %d/%d bytes", written, contentLength);
        broadcastLog("Update error: Download incomplete", "error");
        broadcastProgress("error", 0, "Download incomplete");
        Update.abort();
        return;
    }
    
    broadcastProgress("flash", 98, "Finalizing...");
    
    if (!Update.end(true)) {
        LOG_E("Update failed: %s", Update.errorString());
        broadcastLog("Update error: Installation failed", "error");
        broadcastProgress("error", 0, "Installation failed");
        return;
    }
    
    LOG_I("OTA update successful!");
    broadcastLog("BrewOS updated successfully! Restarting...", "info");
    broadcastProgress("complete", 100, "Update complete!");
    
    // Give time for the message to be sent
    delay(1000);
    
    // Restart to apply the update
    ESP.restart();
    
    #undef broadcastProgress
}

// =============================================================================
// OTA Update Check - Query GitHub for available updates
// =============================================================================

/**
 * Compare semantic version strings (e.g., "0.4.4" vs "0.4.5")
 * Returns: -1 if v1 < v2, 0 if equal, 1 if v1 > v2
 */
static int compareVersions(const String& v1, const String& v2) {
    int major1 = 0, minor1 = 0, patch1 = 0;
    int major2 = 0, minor2 = 0, patch2 = 0;
    
    // Parse v1 (skip leading 'v' if present)
    String ver1 = v1.startsWith("v") ? v1.substring(1) : v1;
    sscanf(ver1.c_str(), "%d.%d.%d", &major1, &minor1, &patch1);
    
    // Parse v2
    String ver2 = v2.startsWith("v") ? v2.substring(1) : v2;
    sscanf(ver2.c_str(), "%d.%d.%d", &major2, &minor2, &patch2);
    
    if (major1 != major2) return major1 < major2 ? -1 : 1;
    if (minor1 != minor2) return minor1 < minor2 ? -1 : 1;
    if (patch1 != patch2) return patch1 < patch2 ? -1 : 1;
    return 0;
}

void WebServer::checkForUpdates() {
    LOG_I("Checking for updates...");
    broadcastLog("Checking for updates...", "info");
    
    // Query GitHub API for latest release
    // API endpoint: https://api.github.com/repos/OWNER/REPO/releases/latest
    String apiUrl = "https://api.github.com/repos/" GITHUB_OWNER "/" GITHUB_REPO "/releases/latest";
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);  // 10 second timeout
    
    if (!http.begin(apiUrl)) {
        LOG_E("Failed to connect to GitHub API");
        broadcastLog("Update check failed: Cannot connect to GitHub", "error");
        return;
    }
    
    // GitHub API requires User-Agent and Accept headers
    http.addHeader("User-Agent", "BrewOS-ESP32/" ESP32_VERSION);
    http.addHeader("Accept", "application/vnd.github.v3+json");
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        LOG_E("GitHub API error: %d", httpCode);
        broadcastLog("Update check failed: HTTP %d", "error", httpCode);
        http.end();
        return;
    }
    
    // Parse JSON response
    String payload = http.getString();
    http.end();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        LOG_E("JSON parse error: %s", error.c_str());
        broadcastLog("Update check failed: Invalid response", "error");
        return;
    }
    
    // Extract release information
    String latestVersion = doc["tag_name"] | "";
    String releaseName = doc["name"] | "";
    String releaseBody = doc["body"] | "";
    bool prerelease = doc["prerelease"] | false;
    String publishedAt = doc["published_at"] | "";
    
    if (latestVersion.isEmpty()) {
        LOG_E("No version found in release");
        broadcastLog("Update check failed: No version found", "error");
        return;
    }
    
    // Strip 'v' prefix for comparison
    String latestVersionNum = latestVersion.startsWith("v") ? latestVersion.substring(1) : latestVersion;
    String currentVersion = ESP32_VERSION;
    
    LOG_I("Current: %s, Latest: %s", currentVersion.c_str(), latestVersionNum.c_str());
    
    // Compare versions
    int cmp = compareVersions(currentVersion, latestVersionNum);
    bool updateAvailable = (cmp < 0);
    
    // Check for both ESP32 and Pico assets
    int esp32AssetSize = 0;
    int picoAssetSize = 0;
    bool esp32AssetFound = false;
    bool picoAssetFound = false;
    
    uint8_t machineType = State.getMachineType();
    const char* picoAssetName = getPicoAssetName(machineType);
    
    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        String assetName = asset["name"] | "";
        if (assetName == GITHUB_ESP32_ASSET) {
            esp32AssetSize = asset["size"] | 0;
            esp32AssetFound = true;
        }
        if (picoAssetName && assetName == picoAssetName) {
            picoAssetSize = asset["size"] | 0;
            picoAssetFound = true;
        }
    }
    
    // Determine if combined update is available
    // Both assets must exist for a proper combined update
    bool combinedUpdateAvailable = updateAvailable && esp32AssetFound && picoAssetFound;
    
    // Broadcast result to WebSocket clients
    JsonDocument result;
    result["type"] = "update_check_result";
    result["updateAvailable"] = updateAvailable;
    result["combinedUpdateAvailable"] = combinedUpdateAvailable;
    result["currentVersion"] = currentVersion;
    result["currentPicoVersion"] = State.getPicoVersion();
    result["latestVersion"] = latestVersionNum;
    result["releaseName"] = releaseName;
    result["prerelease"] = prerelease;
    result["publishedAt"] = publishedAt;
    result["esp32AssetSize"] = esp32AssetSize;
    result["esp32AssetFound"] = esp32AssetFound;
    result["picoAssetSize"] = picoAssetSize;
    result["picoAssetFound"] = picoAssetFound;
    result["picoAssetName"] = picoAssetName ? picoAssetName : "unknown";
    result["machineType"] = machineType;
    
    // Truncate changelog if too long
    if (releaseBody.length() > 500) {
        releaseBody = releaseBody.substring(0, 497) + "...";
    }
    result["changelog"] = releaseBody;
    
    String response;
    serializeJson(result, response);
    _ws.textAll(response);
    
    // Log result
    if (updateAvailable && combinedUpdateAvailable) {
        broadcastLog("BrewOS %s available (current: %s)", latestVersionNum.c_str(), currentVersion.c_str());
    } else if (updateAvailable) {
        // Update available but missing some assets - log internally only
        LOG_W("Update available but missing assets: esp32=%d, pico=%d", esp32AssetFound, picoAssetFound);
        broadcastLog("BrewOS %s available (current: %s)", latestVersionNum.c_str(), currentVersion.c_str());
    } else {
        broadcastLog("BrewOS is up to date (%s)", currentVersion.c_str());
    }
}

// =============================================================================
// Pico GitHub OTA - Download and install Pico firmware from GitHub releases
// =============================================================================

const char* WebServer::getPicoAssetName(uint8_t machineType) {
    switch (machineType) {
        case 1: return GITHUB_PICO_DUAL_BOILER_ASSET;
        case 2: return GITHUB_PICO_SINGLE_BOILER_ASSET;
        case 3: return GITHUB_PICO_HEAT_EXCHANGER_ASSET;
        default: return nullptr;
    }
}

void WebServer::startPicoGitHubOTA(const String& version) {
    LOG_I("Starting Pico GitHub OTA for version: %s", version.c_str());
    
    // Get machine type from StateManager
    uint8_t machineType = State.getMachineType();
    const char* picoAsset = getPicoAssetName(machineType);
    
    if (!picoAsset) {
        LOG_E("Unknown machine type: %d - cannot determine Pico firmware", machineType);
        broadcastLog("Update error: Device not ready", "error");
        return;
    }
    
    LOG_I("Pico asset: %s", picoAsset);
    
    // Build the GitHub release download URL using stack buffer to avoid PSRAM
    char tag[32];
    if (strcmp(version.c_str(), "dev-latest") != 0 && strncmp(version.c_str(), "v", 1) != 0) {
        snprintf(tag, sizeof(tag), "v%s", version.c_str());
    } else {
        strncpy(tag, version.c_str(), sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';
    }
    
    char downloadUrl[256];
    snprintf(downloadUrl, sizeof(downloadUrl), 
             "https://github.com/" GITHUB_OWNER "/" GITHUB_REPO "/releases/download/%s/%s", 
             tag, picoAsset);
    
    LOG_I("Pico download URL: %s", downloadUrl);
    
    // Use static helper for OTA progress (broadcastOtaProgress defined above)
    #define broadcastProgress(stage, progress, message) broadcastOtaProgress(&_ws, stage, progress, message)
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(60000);  // 60 second timeout for larger Pico firmware
    
    if (!http.begin(downloadUrl)) {
        LOG_E("Failed to connect to GitHub");
        broadcastLog("Update error: Cannot connect to server", "error");
        broadcastProgress("error", 0, "Connection failed");
        return;
    }
    
    http.addHeader("User-Agent", "BrewOS-ESP32/" ESP32_VERSION);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        LOG_E("HTTP error: %d", httpCode);
        const char* errorMsg = (httpCode == 404) ? "Update not found for this version" : "Download failed";
        broadcastLog("Update error: %s", "error", errorMsg);
        broadcastProgress("error", 0, errorMsg);
        http.end();
        return;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0 || contentLength > OTA_MAX_SIZE) {
        LOG_E("Invalid content length: %d", contentLength);
        broadcastLog("Update error: Invalid firmware", "error");
        broadcastProgress("error", 0, "Invalid firmware");
        http.end();
        return;
    }
    
    LOG_I("Pico firmware size: %d bytes", contentLength);
    broadcastProgress("download", 10, "Downloading...");
    
    // Save to LittleFS first
    File firmwareFile = LittleFS.open(OTA_FILE_PATH, "w");
    if (!firmwareFile) {
        LOG_E("Failed to create firmware file");
        broadcastLog("Update error: Storage full", "error");
        broadcastProgress("error", 0, "Storage full");
        http.end();
        return;
    }
    
    // Stream to file
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    size_t written = 0;
    int lastProgress = 0;
    
    while (http.connected() && written < (size_t)contentLength) {
        size_t available = stream->available();
        if (available > 0) {
            size_t toRead = min(available, sizeof(buffer));
            size_t bytesRead = stream->readBytes(buffer, toRead);
            
            if (bytesRead > 0) {
                size_t bytesWritten = firmwareFile.write(buffer, bytesRead);
                if (bytesWritten != bytesRead) {
                    LOG_E("File write error");
                    broadcastLog("Update error: Write failed", "error");
                    broadcastProgress("error", 0, "Write failed");
                    firmwareFile.close();
                    http.end();
                    return;
                }
                written += bytesWritten;
                
                // Report progress every 5%
                int progress = 10 + (written * 30) / contentLength;  // 10-40% for download
                if (progress >= lastProgress + 5) {
                    lastProgress = progress;
                    broadcastProgress("download", progress, "Downloading...");
                }
            }
        }
        delay(1);
    }
    
    firmwareFile.close();
    http.end();
    
    if (written != (size_t)contentLength) {
        LOG_E("Download incomplete: %d/%d bytes", written, contentLength);
        broadcastLog("Update error: Download incomplete", "error");
        broadcastProgress("error", 0, "Download incomplete");
        return;
    }
    
    broadcastProgress("flash", 40, "Installing...");
    
    // Now flash to Pico (reuse existing logic)
    File flashFile = LittleFS.open(OTA_FILE_PATH, "r");
    if (!flashFile) {
        LOG_E("Failed to open firmware file for flashing");
        broadcastLog("Update error: Cannot read firmware", "error");
        broadcastProgress("error", 0, "Cannot read firmware");
        return;
    }
    
    size_t firmwareSize = flashFile.size();
    
    // Send bootloader command
    broadcastProgress("flash", 42, "Preparing device...");
    if (!_picoUart.sendCommand(MSG_CMD_BOOTLOADER, nullptr, 0)) {
        LOG_E("Failed to send bootloader command");
        broadcastLog("Update error: Device not responding", "error");
        broadcastProgress("error", 0, "Device not responding");
        flashFile.close();
        return;
    }
    
    // Wait for bootloader ACK
    if (!_picoUart.waitForBootloaderAck(3000)) {
        LOG_E("Bootloader ACK timeout");
        broadcastLog("Update error: Device not ready", "error");
        broadcastProgress("error", 0, "Device not ready");
        flashFile.close();
        return;
    }
    
    broadcastProgress("flash", 45, "Installing...");
    
    // Stream to Pico
    bool success = streamFirmwareToPico(flashFile, firmwareSize);
    flashFile.close();
    
    if (!success) {
        LOG_E("Pico firmware streaming failed");
        broadcastLog("Update error: Installation failed", "error");
        broadcastProgress("error", 0, "Installation failed");
        return;
    }
    
    // Reset Pico
    broadcastProgress("flash", 55, "Restarting...");
    delay(1000);
    _picoUart.resetPico();
    
    // Wait for Pico to boot and send MSG_BOOT
    delay(3000);  // Give Pico time to boot
    
    LOG_I("Pico OTA complete!");
    
    #undef broadcastProgress
}

// =============================================================================
// Combined OTA - Update Pico first, then ESP32
// =============================================================================

void WebServer::startCombinedOTA(const String& version) {
    LOG_I("Starting combined OTA for version: %s", version.c_str());
    broadcastLog("Starting BrewOS update to v%s...", version.c_str());
    
    // Define macro for consistent OTA progress broadcasting
    #define broadcastProgress(stage, progress, message) broadcastOtaProgress(&_ws, stage, progress, message)
    // Note: broadcastProgress macro already defined above for startPicoGitHubOTA
    
    // Check machine type is known
    uint8_t machineType = State.getMachineType();
    if (machineType == 0) {
        LOG_E("Machine type unknown - Pico not connected?");
        broadcastLog("Update error: Please ensure the machine is powered on and connected.", "error");
        broadcastProgress("error", 0, "Device not ready");
        return;
    }
    
    broadcastProgress("download", 0, "Preparing update...");
    
    // Step 1: Update Pico (internal module)
    LOG_I("Step 1/2: Updating Pico firmware...");
    startPicoGitHubOTA(version);
    
    // Check if Pico reconnected (crude check - wait and verify)
    delay(5000);  // Wait for internal module to fully boot
    
    if (!_picoUart.isConnected()) {
        LOG_W("Pico not responding after update - continuing with ESP32 update anyway");
        broadcastLog("Finalizing update...", "info");
    } else {
        // Verify Pico version matches
        const char* picoVer = State.getPicoVersion();
        if (picoVer && String(picoVer) != version && String(picoVer) != version.substring(1)) {
            LOG_W("Pico version mismatch after update: %s vs %s", picoVer, version.c_str());
        }
        broadcastLog("Finalizing update...", "info");
    }
    
    broadcastProgress("flash", 60, "Completing update...");
    
    #undef broadcastProgress
    
    // Step 2: Update ESP32 (this will reboot)
    LOG_I("Step 2/2: Updating ESP32 firmware...");
    startGitHubOTA(version);
    
    // Note: startGitHubOTA reboots ESP32, so we won't reach here
}

// =============================================================================
// Version Mismatch Detection
// =============================================================================

bool WebServer::checkVersionMismatch() {
    const char* picoVersion = State.getPicoVersion();
    const char* esp32Version = ESP32_VERSION;
    
    // If Pico version not available, can't check
    if (!picoVersion || picoVersion[0] == '\0') {
        return false;  // No mismatch detected (not fully connected yet)
    }
    
    // Compare versions using stack buffers (strip 'v' prefix if present)
    char picoVer[16];
    char esp32Ver[16];
    strncpy(picoVer, picoVersion[0] == 'v' ? picoVersion + 1 : picoVersion, sizeof(picoVer) - 1);
    picoVer[sizeof(picoVer) - 1] = '\0';
    strncpy(esp32Ver, esp32Version[0] == 'v' ? esp32Version + 1 : esp32Version, sizeof(esp32Ver) - 1);
    esp32Ver[sizeof(esp32Ver) - 1] = '\0';
    
    bool mismatch = (strcmp(picoVer, esp32Ver) != 0);
    
    if (mismatch) {
        LOG_W("Internal version mismatch: %s vs %s", esp32Ver, picoVer);
        
        // Broadcast to clients using stack allocation
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StaticJsonDocument<256> doc;
        #pragma GCC diagnostic pop
        doc["type"] = "version_mismatch";
        doc["currentVersion"] = esp32Ver;
        doc["message"] = "A firmware update is recommended to ensure optimal performance.";
        
        size_t jsonSize = measureJson(doc) + 1;
        char* jsonBuffer = (char*)heap_caps_malloc(jsonSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!jsonBuffer) jsonBuffer = (char*)malloc(jsonSize);
        if (jsonBuffer) {
            serializeJson(doc, jsonBuffer, jsonSize);
            _ws.textAll(jsonBuffer);
            free(jsonBuffer);
        }
    }
    
    return mismatch;
}


