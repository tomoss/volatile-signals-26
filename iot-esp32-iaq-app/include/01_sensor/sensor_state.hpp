#ifndef SENSOR_STATE_HPP
#define SENSOR_STATE_HPP

#include <array>
#include <cstdint>

#include "00_vendor/bsec2.hpp"

using SensorState = std::array<uint8_t, BSEC_MAX_STATE_BLOB_SIZE>;

#endif // SENSOR_STATE_HPP
