#include "02_storage/storage.hpp"

constexpr char STORAGE_BSEC_NAMESPACE[] = "env-sensor";

constexpr char STORAGE_WIFI_NAMESPACE[] = "wifi";
constexpr char STORAGE_KEY_WIFI_SSID[] = "wifi-ssid";
constexpr char STORAGE_KEY_WIFI_PASS[] = "wifi-pass";

constexpr char STORAGE_MQTT_NAMESPACE[] = "mqtt";
constexpr char STORAGE_KEY_MQTT_USER[] = "mqtt-user";
constexpr char STORAGE_KEY_MQTT_PASS[] = "mqtt-pass";

std::optional<SensorState> Storage::loadBsecState(StorageKey p_key) {
    SensorState l_state{};
    size_t l_len = get(STORAGE_BSEC_NAMESPACE, p_key, l_state.data(), l_state.size());
    if (l_len != l_state.size())
        return std::nullopt;
    return l_state;
}

bool Storage::saveBsecState(StorageKey p_key, const SensorState& p_state) {
    return put(STORAGE_BSEC_NAMESPACE, p_key, p_state.data(), p_state.size());
}

std::optional<WifiTypes::Ssid> Storage::loadWifiSSID() {
    WifiTypes::Ssid l_ssid{};
    size_t l_len = get(STORAGE_WIFI_NAMESPACE, STORAGE_KEY_WIFI_SSID, l_ssid.data(), l_ssid.size());
    if (l_len == 0)
        return std::nullopt;
    return l_ssid;
}

bool Storage::saveWifiSSID(const WifiTypes::Ssid& p_ssid) {
    return put(STORAGE_WIFI_NAMESPACE, STORAGE_KEY_WIFI_SSID, p_ssid.data());
}

std::optional<WifiTypes::Password> Storage::loadWifiPass() {
    WifiTypes::Password l_password{};
    size_t l_len = get(STORAGE_WIFI_NAMESPACE, STORAGE_KEY_WIFI_PASS, l_password.data(), l_password.size());
    if (l_len == 0)
        return std::nullopt;
    return l_password;
}

bool Storage::saveWifiPass(const WifiTypes::Password& p_password) {
    return put(STORAGE_WIFI_NAMESPACE, STORAGE_KEY_WIFI_PASS, p_password.data());
}

std::optional<MqttTypes::Username> Storage::loadMqttUsername() {
    MqttTypes::Username l_username{};
    size_t l_len = get(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_USER, l_username.data(), l_username.size());
    if (l_len == 0)
        return std::nullopt;
    return l_username;
}

bool Storage::saveMqttUsername(const MqttTypes::Username& p_username) {
    return put(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_USER, p_username.data());
}

std::optional<MqttTypes::Password> Storage::loadMqttPassword() {
    MqttTypes::Password l_password{};
    size_t l_len = get(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_PASS, l_password.data(), l_password.size());
    if (l_len == 0)
        return std::nullopt;
    return l_password;
}

bool Storage::saveMqttPassword(const MqttTypes::Password& p_password) {
    return put(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_PASS, p_password.data());
}
