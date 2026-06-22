#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <array>
#include <concepts>
#include <cstdint>
#include <optional>

#include "00_vendor/freertos.hpp"
#include "00_vendor/preferences.hpp"
#include "01_sensor/sensor_types.hpp"
#include "03_wifi/wifi_types.hpp"

class Storage {
public:
    static constexpr char STORAGE_BSEC_NAMESPACE[] = "env-sensor";
    static constexpr char STORAGE_KEY_BSEC_STATE_LP[] = "bsec-state-lp";
    static constexpr char STORAGE_KEY_BSEC_STATE_ULP[] = "bsec-state-ulp";

    static constexpr char STORAGE_WIFI_NAMESPACE[] = "wifi";
    static constexpr char STORAGE_KEY_WIFI_SSID[] = "wifi-ssid";
    static constexpr char STORAGE_KEY_WIFI_PASS[] = "wifi-pass";

    using StorageKey = const char*;

    Storage() = default;
    ~Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    std::optional<SensorState> loadBsecState(StorageKey p_key);
    bool saveBsecState(StorageKey p_key, const SensorState& p_state);

    std::optional<WifiTypes::Ssid> loadWifiSSID();
    bool saveWifiSSID(const WifiTypes::Ssid& p_ssid);

    std::optional<WifiTypes::Password> loadWifiPass();
    bool saveWifiPass(const WifiTypes::Password& p_password);

private:
    static SemaphoreHandle_t mutex() {
        static SemaphoreHandle_t s_mutex = xSemaphoreCreateMutex();
        return s_mutex;
    }

    template<typename T>
        requires(std::same_as<T, char> || std::same_as<T, uint8_t>)
    size_t get(const char* p_namespace, const char* p_key, T* p_buf, size_t p_size) {

        xSemaphoreTake(mutex(), portMAX_DELAY);

        Preferences l_preferences;
        l_preferences.begin(p_namespace, true);

        size_t l_len;

        if constexpr (std::same_as<T, char>) {
            l_len = l_preferences.getString(p_key, p_buf, p_size);
        } else {
            l_len = l_preferences.getBytes(p_key, p_buf, p_size);
        }

        l_preferences.end();
        xSemaphoreGive(mutex());

        return l_len;
    }

    template<typename T>
        requires(std::same_as<T, char> || std::same_as<T, uint8_t>)
    bool put(const char* p_namespace, const char* p_key, const T* p_buf, size_t p_size = 0) {
        xSemaphoreTake(mutex(), portMAX_DELAY);

        Preferences l_preferences;
        l_preferences.begin(p_namespace, false);

        bool l_status;

        if constexpr (std::same_as<T, char>) {
            l_status = l_preferences.putString(p_key, p_buf) > 0;
        } else {
            l_status = l_preferences.putBytes(p_key, p_buf, p_size) == p_size;
        }

        l_preferences.end();
        xSemaphoreGive(mutex());

        return l_status;
    }
};

#endif // STORAGE_HPP