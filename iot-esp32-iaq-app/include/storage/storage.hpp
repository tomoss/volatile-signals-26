#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <array>
#include <cstdint>
#include <optional>

#include "claim/claim_store.hpp"
#include "mqtt/mqtt_store.hpp"
#include "sensor/sensor_store.hpp"
#include "utils/mutex.hpp"
#include "vendor/preferences.hpp"
#include "wifi/wifi_store.hpp"

class Storage : public SensorStore, public WifiStore, public MqttStore, public ClaimStore {
public:
    Storage() = default;
    ~Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    [[nodiscard]] bool init() {
        if (!m_mutex.init()) {
            Serial.println("Storage init failed");
            return false;
        }
        return true;
    }

    std::optional<SensorState> loadBsecState(SensorMode p_mode) override;
    bool saveBsecState(SensorMode p_mode, const SensorState& p_state) override;

    std::optional<SensorMode> loadSensorMode() override;
    bool saveSensorMode(SensorMode p_mode) override;

    std::optional<WifiTypes::Ssid> loadWifiSSID() override;
    bool saveWifiSSID(const WifiTypes::Ssid& p_ssid) override;

    std::optional<WifiTypes::Password> loadWifiPass() override;
    bool saveWifiPass(const WifiTypes::Password& p_password) override;

    std::optional<MqttTypes::Host> loadMqttHost() override;
    bool saveMqttHost(const MqttTypes::Host& p_host);

    std::optional<MqttTypes::Port> loadMqttPort() override;
    bool saveMqttPort(MqttTypes::Port p_port);

    std::optional<MqttTypes::Username> loadMqttUsername() override;
    bool saveMqttUsername(const MqttTypes::Username& p_username);

    std::optional<MqttTypes::Password> loadMqttPassword() override;
    bool saveMqttPassword(const MqttTypes::Password& p_password);

    bool loadDeviceClaimStatus() override;
    bool saveDeviceClaimStatus(bool p_claimed) override;

    std::optional<ClaimCode> loadClaimCode() override;
    bool saveClaimCode(const ClaimCode& p_code) override;

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