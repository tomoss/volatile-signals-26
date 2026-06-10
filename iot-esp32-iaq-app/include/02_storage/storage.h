#ifndef STORAGE_H
#define STORAGE_H

#include <array>
#include <concepts>
#include <cstdint>
#include <optional>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Preferences.h>

#include "01_sensor/sensor_types.h"

class Storage {
public:
    static constexpr const char* STORAGE_BSEC_NAMESPACE = "env-sensor";
    static constexpr const char* STORAGE_KEY_BSEC_STATE_LP = "bsec-state-lp";
    static constexpr const char* STORAGE_KEY_BSEC_STATE_ULP = "bsec-state-ulp";

    using StorageKey = const char*;

    Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;
    ~Storage() = default;

    std::optional<SensorState> loadBsecState(StorageKey p_key);
    bool saveBsecState(StorageKey p_key, const SensorState& p_state);

private:
    static SemaphoreHandle_t mutex() {
        static SemaphoreHandle_t s_mutex = xSemaphoreCreateMutex();
        return s_mutex;
    }

    template<typename T>
        requires(std::same_as<T, char> || std::same_as<T, uint8_t>)
    size_t get(const char* p_namespace, const char* p_key, T* p_buf, size_t p_size) {
        xSemaphoreTake(mutex(), portMAX_DELAY);

        Preferences preferences;
        preferences.begin(p_namespace, true);

        size_t len;

        if constexpr (std::same_as<T, char>) {
            len = preferences.getString(p_key, p_buf, p_size);
        } else {
            len = preferences.getBytes(p_key, p_buf, p_size);
        }

        preferences.end();
        xSemaphoreGive(mutex());

        return len;
    }

    template<typename T>
        requires(std::same_as<T, char> || std::same_as<T, uint8_t>)
    bool put(const char* p_namespace, const char* p_key, const T* p_buf, size_t p_size = 0) {
        xSemaphoreTake(mutex(), portMAX_DELAY);

        Preferences preferences;
        preferences.begin(p_namespace, false);

        bool status;

        if constexpr (std::same_as<T, char>) {
            status = preferences.putString(p_key, p_buf) > 0;
        } else {
            status = preferences.putBytes(p_key, p_buf, p_size) == p_size;
        }

        preferences.end();
        xSemaphoreGive(mutex());

        return status;
    }
};

#endif // STORAGE_H