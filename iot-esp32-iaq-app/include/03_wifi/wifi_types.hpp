#ifndef WIFI_TYPES_H
#define WIFI_TYPES_H

#include <array>
#include <cstddef>

namespace WifiTypes {

constexpr size_t SSID_MAX_LEN = 33;
constexpr size_t PASSWORD_MAX_LEN = 65;
constexpr size_t IP_ADDR_MAX_LEN = 16;
constexpr size_t MAC_ADDR_MAX_LEN = 18;

using Ssid = std::array<char, SSID_MAX_LEN>;
using Password = std::array<char, PASSWORD_MAX_LEN>;
using IpAddr = std::array<char, IP_ADDR_MAX_LEN>;
using MacAddr = std::array<char, MAC_ADDR_MAX_LEN>;
using Rssi = int8_t;

} // namespace WifiTypes

#endif // WIFI_TYPES_H