#include "04_utils/mapping_handler.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

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