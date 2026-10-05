#ifndef TELEMETRY_SOURCE_HPP
#define TELEMETRY_SOURCE_HPP

#include "telemetry/telemetry_data.hpp"

class TelemetrySource {
public:
    virtual ~TelemetrySource() = default;
    virtual bool receive(TelemetryEvent& p_event) = 0;
};

#endif // TELEMETRY_SOURCE_HPP
