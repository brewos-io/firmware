# Brew Statistics Feature

This document describes the brew statistics feature, which provides comprehensive tracking and analytics for brew cycles.

**See also:** [Feature Status Table](../../shared/Feature_Status_Table.md) for implementation status.

---

## Overview

The statistics feature provides comprehensive tracking and analytics for brew cycles. Statistics are managed by the **ESP32**, which has access to accurate timestamps via NTP, more storage capacity, and direct integration with the web UI.

**Architecture Decision:** Statistics are tracked on ESP32 (not Pico) because:
- ESP32 has NTP access for accurate timestamps
- ESP32 has more storage (LittleFS) for historical data
- ESP32 directly serves the web UI
- Simplifies cloud sync integration
- Pico focuses on real-time machine control

**Pico's Role:** The Pico reports brew events (start/stop with duration) to ESP32 via the communication protocol. The cleaning counter remains on Pico for safety (triggers cleaning reminders even without ESP32).

---

## Features

### Lifetime Statistics
- **Total Shots**: All-time count of brew cycles
- **Total Steam Cycles**: Steam usage counter
- **Total Energy**: Cumulative kWh consumed
- **Total Runtime**: Hours the machine has been on
- **Average Brew Time**: Calculated average across all brews
- **Min/Max Brew Times**: Shortest and longest brew times

### Time-Based Statistics
- **Daily**: Shots, energy, and brew stats for last 24 hours
- **Weekly**: Rolling 7-day statistics
- **Monthly**: Rolling 30-day statistics

### Power Tracking
- **Real-time Power**: Current power draw in watts
- **Power History**: 5-minute interval samples for 24 hours
- **Energy Consumption**: kWh tracking per day/week/month

### Brew History
- **Last 200 Brews**: Detailed records with timestamps
- **Brew Details**: Duration, weight, ratio, temperature, pressure
- **User Ratings**: 1-5 star ratings for shots

### Maintenance Tracking
- **Backflush Counter**: Shots since last backflush
- **Group Clean Counter**: Shots since last group clean
- **Descale Counter**: Shots since last descale
- **Timestamps**: When each maintenance was performed

---

## Data Structures

### BrewRecord

```cpp
struct BrewRecord {
    uint32_t timestamp;           // Unix timestamp (from NTP)
    uint16_t durationMs;          // Brew duration in milliseconds
    float yieldWeight;            // Output weight (g) if scale connected
    float doseWeight;             // Input dose (g) if captured
    float peakPressure;           // Maximum pressure during brew
    float avgTemperature;         // Average brew temperature
    float avgFlowRate;            // Average flow rate (g/s)
    uint8_t rating;               // User rating (0-5, 0=unrated)
};
```

### PowerSample

```cpp
struct PowerSample {
    uint32_t timestamp;           // Start of interval (Unix timestamp)
    float avgWatts;               // Average power during interval
    float maxWatts;               // Peak power during interval
    float kwhConsumed;            // Energy consumed during interval
};
```

### DailySummary

```cpp
struct DailySummary {
    uint32_t date;                // Unix timestamp at midnight
    uint16_t shotCount;           // Shots that day
    uint32_t totalBrewTimeMs;     // Total brew time
    float totalKwh;               // Total energy consumed
    uint16_t onTimeMinutes;       // Minutes machine was on
    uint8_t steamCycles;          // Steam cycle count
    float avgBrewTimeMs;          // Average brew time
};
```

### LifetimeStats

```cpp
struct LifetimeStats {
    uint32_t totalShots;
    uint32_t totalSteamCycles;
    float totalKwh;
    uint32_t totalOnTimeMinutes;
    uint32_t totalBrewTimeMs;
    float avgBrewTimeMs;
    float minBrewTimeMs;
    float maxBrewTimeMs;
    uint32_t firstShotTimestamp;  // When user started using
};
```

---

## Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| `STATS_MAX_BREW_HISTORY` | 200 | Maximum brew records to store |
| `STATS_MAX_POWER_SAMPLES` | 288 | 5-min intervals for 24 hours |
| `STATS_MAX_DAILY_HISTORY` | 30 | Days of daily summaries |
| `STATS_MIN_BREW_TIME_MS` | 5000 | Minimum brew time to count (5s) |
| `STATS_MAX_BREW_TIME_MS` | 120000 | Maximum brew time (2 min) |
| `STATS_POWER_SAMPLE_INTERVAL` | 300000 | 5 minutes between samples |

---

## API

### ESP32 Statistics Manager

```cpp
// Get complete statistics
void getFullStatistics(FullStatistics& stats) const;

// Record a completed brew
bool recordBrew(uint32_t durationMs, float yieldWeight = 0, ...);

// Update power readings
void updatePower(float watts, bool isOn);

// Record maintenance
void recordMaintenance(const char* type);

// Get history data
void getBrewHistory(JsonArray& arr, size_t maxEntries = 50) const;
void getPowerHistory(JsonArray& arr) const;
void getDailyHistory(JsonArray& arr) const;
void getWeeklyBrewChart(JsonArray& arr) const;
void getHourlyDistribution(JsonArray& arr) const;
```

### REST API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/stats` | GET | Full statistics summary |
| `/api/stats/extended` | GET | Statistics + all history data |
| `/api/stats/brews` | GET | Brew history (limit param) |
| `/api/stats/power` | GET | Power consumption history |
| `/api/stats/reset` | POST | Reset all statistics |

### WebSocket Messages

Statistics are broadcast via WebSocket when they change:

```json
{
  "type": "stats",
  "lifetime": {
    "totalShots": 1234,
    "totalSteamCycles": 567,
    "totalKwh": 45.67,
    "totalOnTimeMinutes": 12345,
    "avgBrewTimeMs": 28500,
    "minBrewTimeMs": 22000,
    "maxBrewTimeMs": 35000
  },
  "daily": {
    "shotCount": 8,
    "totalBrewTimeMs": 228000,
    "avgBrewTimeMs": 28500,
    "totalKwh": 1.23
  },
  "weekly": { ... },
  "monthly": { ... },
  "maintenance": {
    "shotsSinceBackflush": 45,
    "shotsSinceGroupClean": 12,
    "shotsSinceDescale": 234,
    "lastBackflushTimestamp": 1700000000,
    "lastGroupCleanTimestamp": 1700100000,
    "lastDescaleTimestamp": 1699000000
  },
  "sessionShots": 5,
  "sessionStartTimestamp": 1700123456
}
```

---

## Pico Integration

The Pico reports brew completion to ESP32:

```c
// In state.c, state_exit_action(STATE_BREWING)
uint32_t brew_duration = g_brew_stop_time - g_brew_start_time;

// Send brew completion alarm with duration
alarm_payload_t alarm = {
    .code = ALARM_BREW_COMPLETED,
    .severity = ALARM_INFO,
    .active = 1,
    .duration_ms = brew_duration  // Extended field
};
send_alarm(&alarm);

// Cleaning counter (safety - stays on Pico)
cleaning_record_brew_cycle(brew_duration);
```

**Note:** The cleaning counter remains on Pico to ensure cleaning reminders work even if ESP32 is disconnected.

---

## Persistence

Statistics are stored in ESP32's LittleFS:

| File | Contents |
|------|----------|
| `/stats.json` | Lifetime stats, maintenance counters |
| `/brew_history.json` | Last 200 brew records |
| `/power_history.json` | 24 hours of power samples |
| `/daily_history.json` | 30 days of daily summaries |

**Auto-save:** Statistics are saved every 5 minutes and on shutdown.

---

## Web UI

The Statistics page (`/stats`) provides:

### Overview Tab
- Key metrics: Total shots, today's shots, avg brew time, energy used
- Weekly bar chart showing shots per day
- Hourly distribution chart (when you brew most)
- Brewing stats and milestones

### Power Tab
- Energy metrics: Total, today, this week, this month
- 24-hour power consumption line chart
- 30-day energy trends bar chart

### History Tab
- 30-day brew trends chart
- Detailed brew history list with duration, weight, ratio

### Maintenance Tab
- Backflush, group clean, descale counters
- Mark maintenance done buttons
- Maintenance tips and recommendations

---

## Usage Example

### Recording a Brew (ESP32)

```cpp
// Called when Pico reports brew completion
void onBrewCompleted(uint32_t durationMs, float weight, float pressure, float temp) {
    Stats.recordBrew(
        durationMs,
        weight,           // Yield weight from scale
        brewSettings.doseWeight,  // Configured dose
        pressure,         // Peak pressure
        temp,             // Average temperature
        weight > 0 ? weight / (durationMs / 1000.0f) : 0  // Flow rate
    );
}
```

### Fetching Extended Stats (Web UI)

```typescript
const response = await fetch('/api/stats/extended');
const data = await response.json();

// data.stats - Full statistics
// data.weekly - Weekly chart data
// data.hourlyDistribution - When you brew
// data.brewHistory - Recent brews
// data.powerHistory - Power samples
// data.dailyHistory - Daily summaries
```

---

## Design Notes

### Why ESP32?

1. **Accurate Timestamps**: ESP32 syncs with NTP for real date/time
2. **More Storage**: LittleFS provides ample space for history
3. **Direct UI Access**: No protocol overhead to serve data
4. **Cloud Ready**: Easy to sync stats to cloud service
5. **Pico Simplicity**: Pico focuses on real-time control

### Cleaning Counter Exception

The cleaning counter remains on Pico because:
- It's a safety feature (triggers cleaning reminders)
- Must work even if ESP32 disconnects
- Simple counter doesn't need timestamps

---

## Potential Enhancements

1. **Advanced Analytics**:
   - Brew time distribution histogram
   - Peak usage times analysis
   - Trend predictions

2. **Export/Import**:
   - Export statistics to CSV/JSON
   - Backup/restore functionality
   - Cloud backup integration

3. **Visualizations**:
   - Interactive charts with zoom
   - Comparison views (week over week)
   - Heat maps for usage patterns

4. **Notifications**:
   - Milestone achievements
   - Maintenance reminders
   - Usage summaries

---

## Summary

The enhanced statistics feature provides:

- **Comprehensive tracking**: Lifetime, daily, weekly, monthly stats
- **Power monitoring**: Real-time and historical energy consumption
- **Detailed history**: Last 200 brews with full metrics
- **Visual analytics**: Charts for trends, distribution, and patterns
- **Maintenance tracking**: Counters and timestamps for cleaning tasks
- **ESP32-based**: Accurate timestamps, ample storage, direct UI access
- **REST API**: Full programmatic access to all statistics

**Key Design Decision:** Statistics live on ESP32 for accurate timestamps, more storage, and easier UI/cloud integration. Pico only reports brew events and maintains the safety-critical cleaning counter.
