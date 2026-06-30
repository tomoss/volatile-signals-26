#ifndef SENSOR_DATA_HPP
#define SENSOR_DATA_HPP

#include <cmath>
#include <ctime>

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

#endif // SENSOR_DATA_HPP