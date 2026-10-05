#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include "telemetry/telemetry_data.hpp"

class TelemetryDataConsumer {
public:
    virtual ~TelemetryDataConsumer() = default;
    virtual void update(const TelemetryData& p_data) = 0;
};

class TelemetryInfoConsumer {
public:
    virtual ~TelemetryInfoConsumer() = default;
    virtual void update(const TelemetryInfo& p_info) = 0;
};

#endif // CONSUMER_HPP