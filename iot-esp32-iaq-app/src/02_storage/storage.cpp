#include "02_storage/storage.hpp"
#include "00_vendor/arduino.hpp"

constexpr char STORAGE_BSEC_NAMESPACE[] = "env-sensor";
constexpr char STORAGE_KEY_BSEC_STATE_LP[] = "bsec-state-lp";
constexpr char STORAGE_KEY_BSEC_STATE_ULP[] = "bsec-state-ulp";

constexpr char STORAGE_WIFI_NAMESPACE[] = "wifi";
constexpr char STORAGE_KEY_WIFI_SSID[] = "wifi-ssid";
constexpr char STORAGE_KEY_WIFI_PASS[] = "wifi-pass";

constexpr char STORAGE_MQTT_NAMESPACE[] = "mqtt";
constexpr char STORAGE_KEY_MQTT_HOST[] = "mqtt-host";
constexpr char STORAGE_KEY_MQTT_PORT[] = "mqtt-port";
constexpr char STORAGE_KEY_MQTT_USER[] = "mqtt-user";
constexpr char STORAGE_KEY_MQTT_PASS[] = "mqtt-pass";

constexpr char STORAGE_DEVICE_NAMESPACE[] = "device";
constexpr char STORAGE_KEY_CLAIM_STATUS[] = "claim-status";
constexpr char STORAGE_KEY_CLAIM_CODE[] = "claim-code";

static const char* sensorModeToKey(SensorMode p_mode) {
    switch (p_mode) {
    case SensorMode::LowPower:
        return STORAGE_KEY_BSEC_STATE_LP;
    case SensorMode::UltraLowPower:
        return STORAGE_KEY_BSEC_STATE_ULP;
    default:
        Serial.println("sensorModeToKey: unhandled SensorMode, defaulting to LP key");
        configASSERT(false);
        return STORAGE_KEY_BSEC_STATE_LP;
    }
}

std::optional<SensorState> Storage::loadBsecState(SensorMode p_mode) {
    SensorState l_state{};
    size_t l_len = get(STORAGE_BSEC_NAMESPACE, sensorModeToKey(p_mode), l_state.data(), l_state.size());
    if (l_len != l_state.size())
        return std::nullopt;
    return l_state;
}

bool Storage::saveBsecState(SensorMode p_mode, const SensorState& p_state) {
    return put(STORAGE_BSEC_NAMESPACE, sensorModeToKey(p_mode), p_state.data(), p_state.size());
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

std::optional<MqttTypes::Host> Storage::loadMqttHost() {
    MqttTypes::Host l_host{};
    size_t l_len = get(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_HOST, l_host.data(), l_host.size());
    if (l_len == 0)
        return std::nullopt;
    return l_host;
}

bool Storage::saveMqttHost(const MqttTypes::Host& p_host) {
    return put(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_HOST, p_host.data());
}

std::optional<MqttTypes::Port> Storage::loadMqttPort() {
    MqttTypes::Port l_port = get(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_PORT);
    if (l_port == 0)
        return std::nullopt;
    return l_port;
}

bool Storage::saveMqttPort(MqttTypes::Port p_port) {
    return put(STORAGE_MQTT_NAMESPACE, STORAGE_KEY_MQTT_PORT, p_port);
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

bool Storage::loadDeviceClaimStatus() {
    return get(STORAGE_DEVICE_NAMESPACE, STORAGE_KEY_CLAIM_STATUS, false);
}

bool Storage::saveDeviceClaimStatus(bool p_claimed) {
    return put(STORAGE_DEVICE_NAMESPACE, STORAGE_KEY_CLAIM_STATUS, p_claimed);
}

std::optional<ClaimCode> Storage::loadClaimCode() {
    ClaimCode l_code{};
    size_t l_len = get(STORAGE_DEVICE_NAMESPACE, STORAGE_KEY_CLAIM_CODE, l_code.data(), l_code.size());
    if (l_len == 0)
        return std::nullopt;
    return l_code;
}

bool Storage::saveClaimCode(const ClaimCode& p_code) {
    return put(STORAGE_DEVICE_NAMESPACE, STORAGE_KEY_CLAIM_CODE, p_code.data());
}