#ifndef WIFI_ADAPTER_HPP
#define WIFI_ADAPTER_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/wifi.hpp"
#include "03_wifi/wifi_types.hpp"

#include <array>
#include <functional>

class WifiAdapter {
public:
    using WifiEventCallback = std::function<void(WiFiEvent_t, WiFiEventInfo_t)>;

    WifiAdapter() = default;
    ~WifiAdapter() = default;
    WifiAdapter(const WifiAdapter&) = delete;
    WifiAdapter& operator=(const WifiAdapter&) = delete;
    WifiAdapter(WifiAdapter&&) = delete;
    WifiAdapter& operator=(WifiAdapter&&) = delete;

    [[nodiscard]] bool init();
    void setWifiCallback(WifiEventCallback p_callback);
    void setCredentials(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password);

    bool connect();
    bool reconnect();
    // If p_wifiOff is true, the WiFi radio will be turned off. Otherwise, it will remain on.
    bool disconnect(bool p_wifiOff = false);

    WifiTypes::Rssi getRSSI() const;
    WifiTypes::Ssid getSSID() const;
    WifiTypes::IpAddr getIPAddress() const;
    WifiTypes::MacAddr getMACAddress() const;

private:
    WifiTypes::Ssid m_ssid = {};
    WifiTypes::Password m_password = {};

    WifiEventCallback m_callback;
};

#endif // WIFI_ADAPTER_HPP