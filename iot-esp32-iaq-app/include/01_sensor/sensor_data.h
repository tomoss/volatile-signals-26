#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <cmath>

enum class IAQAccuracy : int { Unreliable = 0, Low = 1, Medium = 2, High = 3 };

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
    unsigned long timestamp = 0;
};

#endif /* SENSOR_DATA_H */