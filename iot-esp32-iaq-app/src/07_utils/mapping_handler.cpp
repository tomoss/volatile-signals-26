#include "07_utils/mapping_handler.hpp"

#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

Storage::StorageKey MappingHandler::sensorModeToStorageKey(const SensorMode p_sensorMode) {
    switch (p_sensorMode) {
    case SensorMode::LowPower:
        return Storage::STORAGE_KEY_BSEC_STATE_LP;
    case SensorMode::UltraLowPower:
        return Storage::STORAGE_KEY_BSEC_STATE_ULP;
    default:
        Serial.println("sensorModeToStorageKey: unhandled SensorMode, defaulting to LP key");
        configASSERT(false);
        return Storage::STORAGE_KEY_BSEC_STATE_LP;
    }
}