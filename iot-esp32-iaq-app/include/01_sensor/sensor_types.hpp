#ifndef SENSOR_TYPES_HPP
#define SENSOR_TYPES_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <variant>

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

#endif // SENSOR_TYPES_HPP
