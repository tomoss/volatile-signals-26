#ifndef MQTT_TYPES_HPP
#define MQTT_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace MqttTypes {

constexpr size_t HOST_MAX_LEN = 64;
constexpr size_t USERNAME_MAX_LEN = 64;
constexpr size_t PASSWORD_MAX_LEN = 64;

using Host = std::array<char, HOST_MAX_LEN>;
using Port = uint16_t;
using Username = std::array<char, USERNAME_MAX_LEN>;
using Password = std::array<char, PASSWORD_MAX_LEN>;

} // namespace MqttTypes

#endif // MQTT_TYPES_HPP
