#include "02_storage/storage.hpp"

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