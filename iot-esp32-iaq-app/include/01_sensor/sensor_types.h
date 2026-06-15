#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <array>

#include "00_vendor/bsec2.h"

using SensorState = std::array<uint8_t, BSEC_MAX_STATE_BLOB_SIZE>;

enum class SensorMode : uint8_t { Disabled, UltraLowPower, LowPower, Continuous };

#endif // SENSOR_TYPES_H
