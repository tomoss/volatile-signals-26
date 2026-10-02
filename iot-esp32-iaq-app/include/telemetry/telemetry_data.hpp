#ifndef TELEMETRY_DATA_HPP
#define TELEMETRY_DATA_HPP

#include <cmath>
#include <cstdint>
#include <ctime>
#include <variant>

struct TelemetryData {
    float iaq = NAN;
    float co2 = NAN;
    float voc = NAN;
    float temp = NAN;
    float hum = NAN;
    float pressure = NAN;
    uint8_t iaqAccuracy = 0;
    time_t timestamp = 0;
};

struct TelemetryInfo {
    uint8_t sensorMode = 0;
};

using TelemetryEvent = std::variant<TelemetryData, TelemetryInfo>;

#endif // TELEMETRY_DATA_HPP
