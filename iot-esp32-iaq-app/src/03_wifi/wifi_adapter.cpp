#include "03_wifi/wifi_adapter.hpp"

bool WifiAdapter::init() {

    WiFi.onEvent([this](WiFiEvent_t p_event, WiFiEventInfo_t p_info) {
        if (m_callback != nullptr) {
            m_callback(p_event, p_info);
        } else {
            Serial.println("WifiAdapter callback not set !");
        }
    });

    // Arduino-ESP32's own auto-reconnect is on by default and would retry on every
    // disconnect independently of (and much faster than) our SM/timer-driven retries.
    // We own reconnection timing entirely; disable theirs so there's a single authority.
    WiFi.setAutoReconnect(false);

    return WiFi.mode(WIFI_STA);
}

void WifiAdapter::setCredentials(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password) {
    m_ssid = p_ssid;
    m_password = p_password;
}

void WifiAdapter::setWifiCallback(WifiEventCallback p_callback) {
    m_callback = std::move(p_callback);
}

bool WifiAdapter::connect() {
    Serial.printf("Connecting to %s...\n", m_ssid.data());

    if (WL_CONNECT_FAILED == WiFi.begin(m_ssid.data(), m_password.data())) {
        return false;
    }
    return true;
}

bool WifiAdapter::reconnect() {
    Serial.printf("Reconnecting to %s...\n", m_ssid.data());

    if (false == disconnect(false)) {
        return false;
    }

    // vTaskDelay(pdMS_TO_TICKS(200)); // let the WiFi stack settle before reconnecting
    if (WL_CONNECT_FAILED == WiFi.begin(m_ssid.data(), m_password.data())) {
        return false;
    }

    return true;
}

bool WifiAdapter::disconnect(bool p_wifiOff) {
    return WiFi.disconnect(p_wifiOff);
}

WifiTypes::Rssi WifiAdapter::getRSSI() const {
    return WiFi.RSSI();
}

WifiTypes::Ssid WifiAdapter::getSSID() const {
    WifiTypes::Ssid l_ssid{};
    WiFi.SSID().toCharArray(l_ssid.data(), l_ssid.size());
    return l_ssid;
}

WifiTypes::IpAddr WifiAdapter::getIPAddress() const {
    WifiTypes::IpAddr l_ipAddr{};
    WiFi.localIP().toString().toCharArray(l_ipAddr.data(), l_ipAddr.size());
    return l_ipAddr;
}

WifiTypes::MacAddr WifiAdapter::getMACAddress() const {
    WifiTypes::MacAddr l_macAddr{};
    WiFi.macAddress().toCharArray(l_macAddr.data(), l_macAddr.size());
    return l_macAddr;
}