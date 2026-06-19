#ifndef WIFI_ADAPTER_H
#define WIFI_ADAPTER_H

#include "00_vendor/arduino.hpp"
#include "00_vendor/wifi.hpp"
#include "03_wifi/wifi_types.hpp"

#include <array>
#include <functional>

class WifiAdapter {
public:
    using WifiEventCallback = std::function<void(WiFiEvent_t, WiFiEventInfo_t)>;

    bool init();

    void setCredentials(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password);
    void setWifiCallback(WifiEventCallback p_callback);

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

#endif // WIFI_ADAPTER_H