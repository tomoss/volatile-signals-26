#ifndef SENSOR_TYPES_HPP
#define SENSOR_TYPES_HPP

#include <array>

#include "00_vendor/bsec2.hpp"

using SensorState = std::array<uint8_t, BSEC_MAX_STATE_BLOB_SIZE>;

enum class SensorMode : uint8_t { Disabled = 0, UltraLowPower = 1, LowPower = 2, Continuous = 3 };

#endif // SENSOR_TYPES_HPP
