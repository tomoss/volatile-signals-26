#ifndef MAC_ADDRESS_HPP
#define MAC_ADDRESS_HPP

#include <array>
#include <cstdint>
#include <cstdio>

#include <esp_mac.h>

using MacAddress = std::array<char, 18>;

// esp_read_mac() reads the factory-burned MAC from eFuse directly, so it's valid
// immediately at boot - unlike WiFi.macAddress(), it doesn't need the STA netif to be up.
inline MacAddress readMacAddress() {
    std::array<uint8_t, 6> l_mac{};
    esp_read_mac(l_mac.data(), ESP_MAC_WIFI_STA);

    MacAddress l_macAddress{};
    snprintf(l_macAddress.data(),
             l_macAddress.size(),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);
    return l_macAddress;
}

#endif // MAC_ADDRESS_HPP
