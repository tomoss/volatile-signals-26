#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <array>
#include <cstdint>
#include <optional>

#include "00_vendor/preferences.hpp"
#include "01_sensor/sensor_types.hpp"
#include "03_wifi/wifi_types.hpp"
#include "04_mqtt/mqtt_types.hpp"
#include "07_utils/claim_code.hpp"
#include "07_utils/mutex.hpp"

class Storage {
public:
    Storage() = default;
    ~Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    [[nodiscard]] bool init() {
        if (!m_mutex.init()) {
            return false;
        }
        return true;
    }

    std::optional<SensorState> loadBsecState(SensorMode p_mode);
    bool saveBsecState(SensorMode p_mode, const SensorState& p_state);

    std::optional<WifiTypes::Ssid> loadWifiSSID();
    bool saveWifiSSID(const WifiTypes::Ssid& p_ssid);

    std::optional<WifiTypes::Password> loadWifiPass();
    bool saveWifiPass(const WifiTypes::Password& p_password);

    std::optional<MqttTypes::Host> loadMqttHost();
    bool saveMqttHost(const MqttTypes::Host& p_host);

    std::optional<MqttTypes::Port> loadMqttPort();
    bool saveMqttPort(MqttTypes::Port p_port);

    std::optional<MqttTypes::Username> loadMqttUsername();
    bool saveMqttUsername(const MqttTypes::Username& p_username);

    std::optional<MqttTypes::Password> loadMqttPassword();
    bool saveMqttPassword(const MqttTypes::Password& p_password);

    // Whether the device has been claimed by a user account yet. Defaults to false (unset).
    bool loadDeviceClaimStatus();
    bool saveDeviceClaimStatus(bool p_claimed);

    std::optional<ClaimCode> loadClaimCode();
    bool saveClaimCode(const ClaimCode& p_code);

private:
    template<typename Func>
    auto withPreferences(const char* p_namespace, bool p_readOnly, Func&& p_func) {
        const MutexGuard l_guard(m_mutex);

        Preferences l_preferences;
        l_preferences.begin(p_namespace, p_readOnly);

        auto l_result = p_func(l_preferences);

        l_preferences.end();

        return l_result;
    }

    size_t get(const char* p_namespace, const char* p_key, char* p_buf, size_t p_size) {
        return withPreferences(p_namespace, true, [&](Preferences& p_preferences) {
            return p_preferences.getString(p_key, p_buf, p_size);
        });
    }

    size_t get(const char* p_namespace, const char* p_key, uint8_t* p_buf, size_t p_size) {
        return withPreferences(p_namespace, true, [&](Preferences& p_preferences) {
            return p_preferences.getBytes(p_key, p_buf, p_size);
        });
    }

    uint16_t get(const char* p_namespace, const char* p_key) {
        return withPreferences(p_namespace, true, [&](Preferences& p_preferences) {
            return p_preferences.getUShort(p_key, 0);
        });
    }

    bool get(const char* p_namespace, const char* p_key, bool p_default) {
        return withPreferences(p_namespace, true, [&](Preferences& p_preferences) {
            return p_preferences.getBool(p_key, p_default);
        });
    }

    bool put(const char* p_namespace, const char* p_key, const char* p_buf) {
        return withPreferences(p_namespace, false, [&](Preferences& p_preferences) {
            return p_preferences.putString(p_key, p_buf) > 0;
        });
    }

    bool put(const char* p_namespace, const char* p_key, const uint8_t* p_buf, size_t p_size) {
        return withPreferences(p_namespace, false, [&](Preferences& p_preferences) {
            return p_preferences.putBytes(p_key, p_buf, p_size) == p_size;
        });
    }

    bool put(const char* p_namespace, const char* p_key, uint16_t p_value) {
        return withPreferences(p_namespace, false, [&](Preferences& p_preferences) {
            return p_preferences.putUShort(p_key, p_value) > 0;
        });
    }

    bool put(const char* p_namespace, const char* p_key, bool p_value) {
        return withPreferences(p_namespace, false, [&](Preferences& p_preferences) {
            return p_preferences.putBool(p_key, p_value) > 0;
        });
    }

    Mutex m_mutex;
};

#endif // STORAGE_HPP