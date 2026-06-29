#ifndef MQTT_TYPES_HPP
#define MQTT_TYPES_HPP

#include <array>
#include <cstddef>

namespace MqttTypes {

constexpr size_t USERNAME_MAX_LEN = 64;
constexpr size_t PASSWORD_MAX_LEN = 65;

using Username = std::array<char, USERNAME_MAX_LEN>;
using Password = std::array<char, PASSWORD_MAX_LEN>;

} // namespace MqttTypes

#endif // MQTT_TYPES_HPP
