#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

namespace BrewOS {

// =============================================================================
// CONFIGURATION
// =============================================================================

constexpr size_t STATS_MAX_BREW_HISTORY = 200;       // Max brew records to store
constexpr size_t STATS_MAX_POWER_SAMPLES = 288;      // 5-min intervals for 24 hours
constexpr size_t STATS_MAX_DAILY_HISTORY = 30;       // 30 days of daily summaries
constexpr uint32_t STATS_MIN_BREW_TIME_MS = 10000;   // 10 seconds minimum (filters out flushing)
constexpr uint32_t STATS_MAX_BREW_TIME_MS = 40000;   // 40 seconds maximum (filters out cleaning)
constexpr uint32_t STATS_POWER_SAMPLE_INTERVAL = 300000;  // 5 minutes

// =============================================================================
// DATA STRUCTURES
// =============================================================================

/**
 * Individual brew record with detailed metrics
 */
struct BrewRecord {
    uint32_t timestamp = 0;           // Unix timestamp (from NTP)
    uint16_t durationMs = 0;          // Brew duration in milliseconds
    float yieldWeight = 0.0f;         // Output weight (g) if scale connected
    float doseWeight = 0.0f;          // Input dose (g) if captured
    float peakPressure = 0.0f;        // Maximum pressure during brew
    float avgTemperature = 0.0f;      // Average brew temperature
    float avgFlowRate = 0.0f;         // Average flow rate (g/s)
    uint8_t rating = 0;               // User rating (0-5, 0=unrated)
    
    void toJson(JsonObject& obj) const;
    bool fromJson(JsonObjectConst obj);
    float ratio() const { return doseWeight > 0 ? yieldWeight / doseWeight : 0; }
};

/**
 * Power consumption sample (aggregated over interval)
 */
struct PowerSample {
    uint32_t timestamp = 0;           // Start of interval (Unix timestamp)
    float avgWatts = 0.0f;            // Average power during interval
    float maxWatts = 0.0f;            // Peak power during interval
    float kwhConsumed = 0.0f;         // Energy consumed during interval
    
    void toJson(JsonObject& obj) const;
    bool fromJson(JsonObjectConst obj);
};

/**
 * Daily summary for trend analysis
 */
struct DailySummary {
    uint32_t date = 0;                // Unix timestamp at midnight
    uint16_t shotCount = 0;           // Shots that day
    uint32_t totalBrewTimeMs = 0;     // Total brew time
    float totalKwh = 0.0f;            // Total energy consumed
    uint16_t onTimeMinutes = 0;       // Minutes machine was on
    uint8_t steamCycles = 0;          // Steam cycle count
    float avgBrewTimeMs = 0.0f;       // Average brew time
    
    void toJson(JsonObject& obj) const;
    bool fromJson(JsonObjectConst obj);
};

/**
 * Time-period statistics (calculated from history)
 */
struct PeriodStats {
    uint16_t shotCount = 0;
    uint32_t totalBrewTimeMs = 0;
    float avgBrewTimeMs = 0.0f;
    float minBrewTimeMs = 0.0f;
    float maxBrewTimeMs = 0.0f;
    float totalKwh = 0.0f;
    
    void toJson(JsonObject& obj) const;
};

/**
 * Overall lifetime statistics
 */
struct LifetimeStats {
    uint32_t totalShots = 0;
    uint32_t totalSteamCycles = 0;
    float totalKwh = 0.0f;
    uint32_t totalOnTimeMinutes = 0;
    uint32_t totalBrewTimeMs = 0;
    float avgBrewTimeMs = 0.0f;
    float minBrewTimeMs = 0.0f;
    float maxBrewTimeMs = 0.0f;
    uint32_t firstShotTimestamp = 0;  // When user started using
    
    void toJson(JsonObject& obj) const;
    bool fromJson(JsonObjectConst obj);
};

/**
 * Maintenance tracking
 */
struct MaintenanceStats {
    uint32_t shotsSinceBackflush = 0;
    uint32_t shotsSinceGroupClean = 0;
    uint32_t shotsSinceDescale = 0;
    uint32_t lastBackflushTimestamp = 0;
    uint32_t lastGroupCleanTimestamp = 0;
    uint32_t lastDescaleTimestamp = 0;
    
    void toJson(JsonObject& obj) const;
    bool fromJson(JsonObjectConst obj);
    void recordMaintenance(const char* type, uint32_t timestamp);
};

/**
 * Complete statistics package for web UI
 */
struct FullStatistics {
    LifetimeStats lifetime;
    PeriodStats daily;                // Last 24 hours
    PeriodStats weekly;               // Last 7 days
    PeriodStats monthly;              // Last 30 days
    MaintenanceStats maintenance;
    uint16_t sessionShots = 0;        // Current session
    uint32_t sessionStartTimestamp = 0;
    
    void toJson(JsonObject& obj) const;
};

// =============================================================================
// STATISTICS MANAGER
// =============================================================================

class StatisticsManager {
public:
    static StatisticsManager& getInstance();
    
    // Lifecycle
    void begin();
    void loop();  // Call periodically for power sampling, auto-save
    
    // ==========================================================================
    // BREW RECORDING
    // ==========================================================================
    
    /**
     * Record a completed brew
     * @param durationMs Brew duration in milliseconds
     * @param yieldWeight Output weight in grams (0 if no scale)
     * @param doseWeight Input dose in grams (0 if not captured)
     * @param peakPressure Maximum pressure during brew
     * @param avgTemp Average temperature during brew
     * @param avgFlow Average flow rate
     * @return true if recorded successfully
     */
    bool recordBrew(uint32_t durationMs, float yieldWeight = 0, float doseWeight = 0,
                    float peakPressure = 0, float avgTemp = 0, float avgFlow = 0);
    
    /**
     * Record steam cycle
     */
    void recordSteamCycle();
    
    /**
     * Record maintenance event
     * @param type "backflush", "groupClean", or "descale"
     */
    void recordMaintenance(const char* type);
    
    /**
     * Rate a previous shot
     * @param index 0 = most recent, 1 = previous, etc.
     * @param rating 1-5 stars
     */
    bool rateBrew(uint8_t index, uint8_t rating);
    
    // ==========================================================================
    // POWER TRACKING
    // ==========================================================================
    
    /**
     * Update current power reading (call frequently from status updates)
     * @param watts Current power draw
     * @param isOn Whether machine is powered on
     */
    void updatePower(float watts, bool isOn);
    
    // ==========================================================================
    // DATA ACCESS
    // ==========================================================================
    
    /**
     * Get complete statistics package
     */
    void getFullStatistics(FullStatistics& stats) const;
    
    /**
     * Get lifetime statistics
     */
    const LifetimeStats& getLifetime() const { return _lifetime; }
    
    /**
     * Get maintenance statistics
     */
    const MaintenanceStats& getMaintenance() const { return _maintenance; }
    
    /**
     * Get period statistics (calculated on demand)
     */
    void getDailyStats(PeriodStats& stats) const;
    void getWeeklyStats(PeriodStats& stats) const;
    void getMonthlyStats(PeriodStats& stats) const;
    
    /**
     * Get brew history (most recent first)
     * @param maxEntries Maximum entries to return
     */
    void getBrewHistory(JsonArray& arr, size_t maxEntries = 50) const;
    
    /**
     * Get power samples for chart (last 24 hours)
     */
    void getPowerHistory(JsonArray& arr) const;
    
    /**
     * Get daily summaries for trend chart (last 30 days)
     */
    void getDailyHistory(JsonArray& arr) const;
    
    /**
     * Get weekly data for chart
     */
    void getWeeklyBrewChart(JsonArray& arr) const;
    
    /**
     * Get hourly distribution for chart
     */
    void getHourlyDistribution(JsonArray& arr) const;
    
    // ==========================================================================
    // PERSISTENCE
    // ==========================================================================
    
    /**
     * Force save all statistics
     */
    void save();
    
    /**
     * Reset all statistics (factory reset)
     */
    void resetAll();
    
    // ==========================================================================
    // SESSION MANAGEMENT
    // ==========================================================================
    
    /**
     * Start a new session (called when machine turns on)
     */
    void startSession();
    
    /**
     * End session (called when machine turns off)
     */
    void endSession();
    
    /**
     * Get session shot count
     */
    uint16_t getSessionShots() const { return _sessionShots; }
    
    // ==========================================================================
    // CALLBACKS
    // ==========================================================================
    
    using StatsCallback = std::function<void(const FullStatistics&)>;
    void onStatsChanged(StatsCallback callback);

private:
    StatisticsManager() = default;
    ~StatisticsManager() = default;
    StatisticsManager(const StatisticsManager&) = delete;
    StatisticsManager& operator=(const StatisticsManager&) = delete;
    
    // Data storage
    LifetimeStats _lifetime;
    MaintenanceStats _maintenance;
    
    // Brew history (circular buffer)
    BrewRecord _brewHistory[STATS_MAX_BREW_HISTORY];
    uint16_t _brewHistoryHead = 0;
    uint16_t _brewHistoryCount = 0;
    
    // Power samples (circular buffer for 24 hours)
    PowerSample _powerSamples[STATS_MAX_POWER_SAMPLES];
    uint16_t _powerSamplesHead = 0;
    uint16_t _powerSamplesCount = 0;
    
    // Daily summaries (circular buffer for 30 days)
    DailySummary _dailySummaries[STATS_MAX_DAILY_HISTORY];
    uint8_t _dailySummariesHead = 0;
    uint8_t _dailySummariesCount = 0;
    
    // Current power tracking
    float _currentWatts = 0.0f;
    float _powerSampleSum = 0.0f;
    float _powerSampleMax = 0.0f;
    uint32_t _powerSampleCount = 0;
    uint32_t _lastPowerSampleTime = 0;
    uint32_t _machineOnStartTime = 0;
    bool _machineIsOn = false;
    
    // Session tracking
    uint16_t _sessionShots = 0;
    uint32_t _sessionStartTimestamp = 0;
    
    // Today tracking
    uint32_t _todayStartTimestamp = 0;
    
    // Persistence timing
    uint32_t _lastSaveTime = 0;
    bool _dirty = false;
    static constexpr uint32_t SAVE_INTERVAL = 300000;  // 5 minutes
    
    // Cached period statistics (recalculated only when data changes)
    mutable PeriodStats _cachedDailyStats;
    mutable PeriodStats _cachedWeeklyStats;
    mutable PeriodStats _cachedMonthlyStats;
    mutable uint32_t _cachedDailyTimestamp = 0;
    mutable uint32_t _cachedWeeklyTimestamp = 0;
    mutable uint32_t _cachedMonthlyTimestamp = 0;
    mutable uint32_t _cachedDailyDay = 0;  // Day of year when cached
    mutable bool _statsCacheInvalid = true;  // Invalidate cache when data changes (mutable for const functions)
    
    // Callback
    StatsCallback _onStatsChanged;
    
    // Helper methods
    void loadFromFlash();
    void saveToFlash();
    void saveDailySummary();
    void checkDayChange();
    void addBrewRecord(const BrewRecord& record);
    void addPowerSample(const PowerSample& sample);
    void addDailySummary(const DailySummary& summary);
    void calculatePeriodStats(PeriodStats& stats, uint32_t startTimestamp) const;
    uint32_t getTodayMidnight() const;
    void notifyChange();
    void checkMaintenanceThresholds();
};

} // namespace BrewOS

// Convenience macro
#define Stats BrewOS::StatisticsManager::getInstance()

