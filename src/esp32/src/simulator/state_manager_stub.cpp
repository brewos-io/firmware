/**
 * StateManager Stub for Simulator
 * 
 * Provides minimal implementation for UI testing without full state persistence
 */

#ifdef SIMULATOR

#include "state/state_manager.h"
#include "state/state_types.h"

namespace BrewOS {

StateManager& StateManager::getInstance() {
    static StateManager instance;
    return instance;
}

void StateManager::begin() {
    // No-op for simulator
}

void StateManager::loop() {
    // No-op for simulator
}

void StateManager::saveSettings() {
    // No-op for simulator
}

void StateManager::saveTemperatureSettings() {}
void StateManager::saveBrewSettings() {}
void StateManager::savePowerSettings() {}
void StateManager::saveNetworkSettings() {}
void StateManager::saveTimeSettings() {}
void StateManager::saveMQTTSettings() {}
void StateManager::saveCloudSettings() {}
void StateManager::saveScaleSettings() {}
void StateManager::saveDisplaySettings() {
    // No-op for simulator - just log
    // printf("[Simulator] Display settings saved: brightness=%d, timeout=%d\n", 
    //        _settings.display.brightness, _settings.display.screenTimeout);
}
void StateManager::saveScheduleSettings() {}
void StateManager::saveMachineInfoSettings() {}
void StateManager::saveNotificationSettings() {}
void StateManager::saveSystemSettings() {}
void StateManager::saveUserPreferences() {}

void StateManager::resetSettings() {}
void StateManager::factoryReset() {}

void StateManager::saveStats() {}
void StateManager::recordShot() {}
void StateManager::recordSteamCycle() {}
void StateManager::addPowerUsage(float kwh) {}
void StateManager::recordMaintenance(const char* type) {}

void StateManager::addShotRecord(const ShotRecord& shot) {}
void StateManager::rateShot(uint8_t index, uint8_t rating) {}
void StateManager::clearShotHistory() {}

void StateManager::setMachineState(MachineState newState) {}
void StateManager::setMachineMode(MachineMode newMode) {}

void StateManager::onStateChanged(StateCallback callback) {}
void StateManager::notifyStateChanged() {}

void StateManager::getFullStateJson(JsonDocument& doc) const {
    // Stub - return empty doc
}
void StateManager::getStateJson(JsonObject& obj) const {
    // Stub - no-op
}

} // namespace BrewOS

#endif // SIMULATOR

