# ESP32 State Management

The ESP32 maintains all machine state, settings, statistics, and shot history with persistent storage.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         StateManager                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌───────────┐  │
│   │  Settings   │   │ Statistics  │   │ ShotHistory │   │  Runtime  │  │
│   │    (NVS)    │   │    (NVS)    │   │  (LittleFS) │   │ (Memory)  │  │
│   └─────────────┘   └─────────────┘   └─────────────┘   └───────────┘  │
│         │                 │                 │                 │        │
│         ▼                 ▼                 ▼                 ▼        │
│   ┌─────────────────────────────────────────────────────────────────┐  │
│   │                    Change Notifications                         │  │
│   │         (WebSocket, MQTT, UI, Cloud callbacks)                  │  │
│   └─────────────────────────────────────────────────────────────────┘  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## Data Categories

### 1. Settings (Persisted to NVS)

User-configurable settings that persist across reboots.

| Section | Storage Key Prefix | Description |
|---------|-------------------|-------------|
| `temperature` | `brewSP`, `steamSP`, etc. | Temperature setpoints and offsets |
| `brew` | `bbwEnabled`, `doseWt`, etc. | Brew-by-weight settings |
| `preinfusion` | `piEnabled`, `piOnMs`, `piPauseMs` | Pre-infusion timing |
| `power` | `voltage`, `maxCurr` | Mains voltage and current limits |
| `network` | `wifiSsid`, `hostname` | WiFi configuration |
| `mqtt` | `mqttBrk`, `mqttPort` | MQTT broker settings |
| `cloud` | `cloudUrl`, `devId` | Cloud service configuration |
| `scale` | `scaleAddr`, `scaleName` | Paired BLE scale |
| `display` | `dispBri`, `dispTO` | Display preferences |
| `preferences` | `prefFDoW`, `pref24H`, etc. | User preferences (units, currency) |

**Memory Usage**: ~2KB in NVS

### 2. Statistics (Persisted to NVS)

Counters and accumulators saved periodically.

```cpp
struct Statistics {
    // Lifetime
    uint32_t totalShots;
    uint32_t totalSteamCycles;
    float totalKwh;
    uint32_t totalOnTimeMinutes;
    
    // Daily (auto-reset at midnight)
    uint16_t shotsToday;
    float kwhToday;
    
    // Maintenance tracking
    uint32_t shotsSinceDescale;
    uint32_t shotsSinceGroupClean;
    uint32_t lastDescaleTimestamp;
    // ...
};
```

**Save Frequency**: Every 5 minutes or on significant events

### 3. Shot History (Persisted to LittleFS)

Ring buffer of recent shots stored as JSON.

```cpp
struct ShotRecord {
    uint32_t timestamp;        // Unix timestamp
    float doseWeight;          // Input (g)
    float yieldWeight;         // Output (g)
    uint16_t durationMs;       // Shot time
    float avgFlowRate;         // g/s
    float peakPressure;        // bar
    float avgTemperature;      // °C
    uint8_t rating;            // 0-5 stars
};
```

**Max Records**: 50 (configurable via `MAX_SHOT_HISTORY`)
**Memory Usage**: ~4KB in flash

### 4. User Preferences (Persisted to NVS)

Display and regional preferences, synced with web UI.

```cpp
struct UserPreferences {
    uint8_t firstDayOfWeek;    // 0=Sunday, 1=Monday
    bool use24HourTime;        // 12h vs 24h clock
    uint8_t temperatureUnit;   // 0=celsius, 1=fahrenheit
    float electricityPrice;    // Price per kWh (for energy cost)
    char currency[4];          // Currency code (USD, EUR, GBP, etc.)
    uint8_t lastHeatingStrategy; // 0-3 (remembered for quick on)
    bool initialized;          // True after first browser setup
};
```

**Browser Sync**: If `initialized` is false, the ESP32 will accept initial preferences from the browser based on locale detection (timezone, country, etc.).

### 5. Runtime State (Volatile)

Current machine state, not persisted.

```cpp
struct RuntimeState {
    MachineState state;        // init, heating, ready, brewing, etc.
    MachineMode mode;          // standby, on, eco
    float brewTemp, steamTemp;
    float pressure, flowRate;
    bool shotActive;
    // ...
};
```

## Usage

### Initialization

```cpp
#include "state/state_manager.h"

void setup() {
    // Initialize state manager
    State.begin();
}

void loop() {
    // Periodic housekeeping (auto-save, daily reset)
    State.loop();
}
```

### Accessing Settings

```cpp
// Read settings
float brewTemp = State.settings().temperature.brewSetpoint;
bool bbwEnabled = State.settings().brew.bbwEnabled;

// Modify settings
State.settings().temperature.brewSetpoint = 94.0f;
State.saveTemperatureSettings();  // Persist to NVS
```

### Tracking Statistics

```cpp
// Record a shot (increments counters)
State.recordShot();

// Record maintenance
State.recordMaintenance("descale");  // Resets shotsSinceDescale

// Read stats
uint32_t total = State.stats().totalShots;
```

### Shot Tracking

```cpp
// When brew starts
State.startShot();

// During brewing - updates are automatic via updateTemperatures(), updatePressure()

// When brew ends
State.endShot();  // Saves shot record to history
```

### Change Notifications

```cpp
// Subscribe to settings changes
State.onSettingsChanged([](const Settings& s) {
    Serial.printf("Brew temp changed to %.1f\n", s.temperature.brewSetpoint);
    // Update PID, etc.
});

// Subscribe to shot completion
State.onShotCompleted([](const ShotRecord& shot) {
    Serial.printf("Shot: %.1fg in %dms\n", shot.yieldWeight, shot.durationMs);
    // Send to cloud, log, etc.
});
```

### Serialization for WebSocket

```cpp
// Get full state as JSON
StaticJsonDocument<4096> doc;
State.getFullStateJson(doc);
String json;
serializeJson(doc, json);
webSocket.broadcastTXT(json);

// Get specific section
JsonObject statsObj = doc.createNestedObject("stats");
State.getStatsJson(statsObj);
```

### Applying Settings from Web/Cloud

```cpp
// Handle incoming settings update
void handleSettingsUpdate(const JsonDocument& doc) {
    // Apply all settings
    State.applySettings(doc);
    
    // Or apply specific section
    State.applySettings("temperature", doc["temperature"]);
}
```

## Storage Details

### NVS (Non-Volatile Storage)

- **Namespace**: `settings`, `stats`
- **Key limit**: 15 characters
- **Value types**: Primitives, strings (max 4KB each)
- **Wear leveling**: Handled by ESP-IDF

### LittleFS

- **File**: `/shot_history.json`
- **Format**: JSON array
- **Max size**: ~8KB

## Memory Budget

| Component | RAM | Flash |
|-----------|-----|-------|
| Settings struct | ~500 bytes | - |
| Statistics struct | ~100 bytes | - |
| ShotHistory (50 records) | ~2KB | ~4KB |
| RuntimeState | ~200 bytes | - |
| JSON buffers | ~4KB (temp) | - |
| **Total** | ~3KB static | ~4KB |

## Best Practices

1. **Don't save on every change** - Use `saveTemperatureSettings()` only when user confirms
2. **Batch updates** - Modify multiple fields, then save once
3. **Use callbacks** - Don't poll, subscribe to changes
4. **JSON budgets** - Use `StaticJsonDocument` with appropriate size
5. **Daily reset** - `loop()` handles automatic daily stats reset

## Thread Safety

The StateManager is **not thread-safe**. All access should be from the main loop or use mutex protection if called from tasks.

## Factory Reset

```cpp
// Reset to defaults
State.resetSettings();    // Settings only

State.factoryReset();     // Everything (settings, stats, history)
```

