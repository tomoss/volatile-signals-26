#ifndef TELEMETRY_SINK_HPP
#define TELEMETRY_SINK_HPP

#include "telemetry/telemetry_data.hpp"

class TelemetrySink {
public:
    virtual ~TelemetrySink() = default;
    virtual void enqueue(const TelemetryEvent& p_event) = 0;
};

#endif // TELEMETRY_SINK_HPP
