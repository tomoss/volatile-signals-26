#ifndef MQTT_TYPES_HPP
#define MQTT_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace MqttTypes {

constexpr size_t HOST_MAX_LEN = 64;
constexpr size_t USERNAME_MAX_LEN = 64;
constexpr size_t PASSWORD_MAX_LEN = 64;
// Sized for the SensorData fields the JSON payload emits, plus headroom
constexpr size_t PAYLOAD_MAX_LEN = 192;
// Shared size for every topic buffer (sensor_data, device_health, device_info, device_status,
// command, ota, sensor_info, ...), sized for the longest ones (device_health/device_status):
// "iaq/" (4) + MAC ("AA:BB:CC:DD:EE:FF", 17) + "/device_health" (14) + null terminator (1) = 36,
// rounded up to 40 for alignment/headroom.
constexpr size_t TOPIC_MAX_LEN = 40;

using Host = std::array<char, HOST_MAX_LEN>;
using Port = uint16_t;
using Username = std::array<char, USERNAME_MAX_LEN>;
using Password = std::array<char, PASSWORD_MAX_LEN>;
using Payload = std::array<char, PAYLOAD_MAX_LEN>;
using Topic = std::array<char, TOPIC_MAX_LEN>;

} // namespace MqttTypes

#endif // MQTT_TYPES_HPP
