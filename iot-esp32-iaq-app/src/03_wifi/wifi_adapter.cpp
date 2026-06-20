#include "03_wifi/wifi_adapter.hpp"

constexpr uint32_t MAX_RECONNECT_ATTEMPTS = 5;     // Max number of reconnect before provisioning is triggered
constexpr uint32_t RECONNECT_DELAY_MS = 30 * 1000; // 30 seconds

WifiAdapter::WifiAdapter(Storage& p_storage)
    : m_storage(p_storage) {}

WifiAdapter::~WifiAdapter() {
    if (m_reconnectTimer != nullptr) {
        xTimerDelete(m_reconnectTimer, 0);
        m_reconnectTimer = nullptr;
    }
}

bool WifiAdapter::init() {

    WiFi.onEvent([this](WiFiEvent_t p_event, WiFiEventInfo_t p_info) {
        if (m_wifiApiCallback != nullptr) {
            m_wifiApiCallback(p_event, p_info);
        } else {
            Serial.println("WiFi API callback not set !");
        }
    });

    // Arduino-ESP32's own auto-reconnect is on by default and would retry on every
    // disconnect independently of (and much faster than) our SM/timer-driven retries.
    // We own reconnection timing entirely; disable theirs so there's a single authority.
    WiFi.setAutoReconnect(false);

    m_reconnectTimer = xTimerCreate("wifi_reconnect", pdMS_TO_TICKS(RECONNECT_DELAY_MS), pdFALSE, this, reconnectTimerTimeout);
    if (m_reconnectTimer == nullptr) {
        return false;
    }

    return WiFi.mode(WIFI_STA);
}

void WifiAdapter::reconnectTimerTimeout(TimerHandle_t p_timer) {
    auto* l_self = static_cast<WifiAdapter*>(pvTimerGetTimerID(p_timer));
    if (l_self->m_reconnectTimerCallback) {
        Serial.println("Reconnect timer timeout, trying again...");
        l_self->m_reconnectTimerCallback();
    } else {
        Serial.println("No reconnect timer callback set");
    }
}

bool WifiAdapter::loadCredentials() {
    auto l_ssid = m_storage.loadWifiSSID();
    auto l_pass = m_storage.loadWifiPass();
    if (!l_ssid || !l_pass) {
        Serial.println("No WiFi credentials in storage");
        return false;
    }
    Serial.println("Loaded WiFi credentials from storage");
    m_ssid = *l_ssid;
    m_password = *l_pass;
    return true;
}

void WifiAdapter::setWifiCallback(WifiEventCallback p_callback) {
    m_wifiApiCallback = std::move(p_callback);
}

void WifiAdapter::setProvisioningCallback(ProvisioningCallback p_callback) {
    m_provisioningCallback = std::move(p_callback);
}

void WifiAdapter::notifyProvisioning() const {
    if (m_provisioningCallback) {
        m_provisioningCallback();
    } else {
        Serial.println("No provisioning callback set");
    }
}

void WifiAdapter::setConnectedCallback(ConnectedCallback p_callback) {
    m_connectedCallback = std::move(p_callback);
}

void WifiAdapter::notifyConnected() const {
    if (m_connectedCallback) {
        m_connectedCallback();
    } else {
        Serial.println("No connected callback set");
    }
}

void WifiAdapter::setDisconnectedCallback(DisconnectedCallback p_callback) {
    m_disconnectedCallback = std::move(p_callback);
}

void WifiAdapter::notifyDisconnected() const {
    if (m_disconnectedCallback) {
        m_disconnectedCallback();
    } else {
        Serial.println("No disconnected callback set");
    }
}

void WifiAdapter::recordReconnectAttempt() {
    m_reconnectAttempts++;
}

bool WifiAdapter::maxReconnectAttemptsReached() const {
    return m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS;
}

void WifiAdapter::resetReconnectAttempts() {
    m_reconnectAttempts = 0;
}

void WifiAdapter::setReconnectTimerCallback(ReconnectTimerCallback p_callback) {
    m_reconnectTimerCallback = std::move(p_callback);
}

bool WifiAdapter::startReconnectTimer() const {
    return xTimerStart(m_reconnectTimer, 0) == pdPASS;
}

bool WifiAdapter::connect() {
    if (WL_CONNECT_FAILED == WiFi.begin(m_ssid.data(), m_password.data())) {
        return false;
    }
    return true;
}

// bool WifiAdapter::disconnect(bool p_wifiOff) {
//     return WiFi.disconnect(p_wifiOff);
// }

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