#ifndef WIFI_ADAPTER_HPP
#define WIFI_ADAPTER_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/wifi.hpp"
#include "02_storage/storage.hpp"
#include "03_wifi/wifi_types.hpp"

#include <array>
#include <functional>

class WifiAdapter {
public:
    using WifiEventCallback = std::function<void(WiFiEvent_t, WiFiEventInfo_t)>;
    using ProvisioningCallback = std::function<void()>;
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using ReconnectTimerCallback = std::function<void()>;

    WifiAdapter(Storage& p_storage);
    ~WifiAdapter();
    WifiAdapter(const WifiAdapter&) = delete;
    WifiAdapter& operator=(const WifiAdapter&) = delete;
    WifiAdapter(WifiAdapter&&) = delete;
    WifiAdapter& operator=(WifiAdapter&&) = delete;

    [[nodiscard]] bool init();
    void setWifiCallback(WifiEventCallback p_callback);
    [[nodiscard]] bool loadCredentials();

    void setProvisioningCallback(ProvisioningCallback p_callback);
    void notifyProvisioning() const;

    void setConnectedCallback(ConnectedCallback p_callback);
    void notifyConnected() const;

    void setDisconnectedCallback(DisconnectedCallback p_callback);
    void notifyDisconnected() const;

    // Reconnect-attempt budget, checked by the SM's GuMaxAttemptsReached guard.
    void recordReconnectAttempt();
    bool maxReconnectAttemptsReached() const;
    void resetReconnectAttempts();
    uint8_t getReconnectAttempts() const { return m_reconnectAttempts; }

    // Reconnect timer is created/owned here; setReconnectTimerCallback is the relay slot
    // WifiManager fills in (same shape as setWifiCallback) to react when it fires.
    void setReconnectTimerCallback(ReconnectTimerCallback p_callback);
    bool startReconnectTimer() const;

    [[nodiscard]] bool connect();

    // If p_wifiOff is true, the WiFi radio will be turned off. Otherwise, it will remain on.
    //[[nodiscard]] bool disconnect(bool p_wifiOff = false);

    WifiTypes::Rssi getRSSI() const;
    WifiTypes::Ssid getSSID() const;
    WifiTypes::IpAddr getIPAddress() const;
    WifiTypes::MacAddr getMACAddress() const;

private:
    // Timer callback is static because the timer API doesn't support capturing lambdas or std::function.
    static void reconnectTimerTimeout(TimerHandle_t p_timer);

    WifiTypes::Ssid m_ssid = {};
    WifiTypes::Password m_password = {};

    WifiEventCallback m_wifiApiCallback;
    ProvisioningCallback m_provisioningCallback;
    ConnectedCallback m_connectedCallback;
    DisconnectedCallback m_disconnectedCallback;
    ReconnectTimerCallback m_reconnectTimerCallback;

    uint8_t m_reconnectAttempts = 0;
    TimerHandle_t m_reconnectTimer = nullptr;
    Storage& m_storage;
};

#endif // WIFI_ADAPTER_HPP