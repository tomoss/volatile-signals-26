#ifndef SENSOR_TYPES_HPP
#define SENSOR_TYPES_HPP

#include <array>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <variant>

#include "00_vendor/bsec2.hpp"
#include "09_utils/queue.hpp"

using SensorState = std::array<uint8_t, BSEC_MAX_STATE_BLOB_SIZE>;

enum class SensorMode : uint8_t { Disabled = 0, UltraLowPower = 1, LowPower = 2, Continuous = 3 };

enum class IAQAccuracy : uint8_t { Unreliable = 0, Low = 1, Medium = 2, High = 3 };

struct SensorData {
    float iaq = NAN;
    float co2 = NAN;
    float voc = NAN;
    float temp = NAN;
    float hum = NAN;
    float pressure = NAN;
    float gas = NAN;
    float rawTemp = NAN;
    float rawHum = NAN;
    IAQAccuracy iaqAccuracy = IAQAccuracy::Unreliable;
    time_t timestamp = 0;
};

using SensorEvent = std::variant<SensorData, SensorMode>;

constexpr std::size_t SENSOR_EVENT_QUEUE_LENGTH = 10;

using SensorEventQueue = Queue<SensorEvent, SENSOR_EVENT_QUEUE_LENGTH>;

// Length 1 so a newer request overwrites one not yet applied
using SensorModeRequestQueue = Queue<SensorMode, 1>;

#endif // SENSOR_TYPES_HPP
