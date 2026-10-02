#ifndef WIFI_STORE_HPP
#define WIFI_STORE_HPP

#include <optional>

#include "wifi/wifi_types.hpp"

class WifiStore {
public:
    virtual ~WifiStore() = default;

    virtual std::optional<WifiTypes::Ssid> loadWifiSSID() = 0;
    virtual bool saveWifiSSID(const WifiTypes::Ssid& p_ssid) = 0;

    virtual std::optional<WifiTypes::Password> loadWifiPass() = 0;
    virtual bool saveWifiPass(const WifiTypes::Password& p_password) = 0;
};

#endif // WIFI_STORE_HPP
