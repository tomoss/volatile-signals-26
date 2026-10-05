#ifndef TELEMETRY_PUBLISHER_HPP
#define TELEMETRY_PUBLISHER_HPP

#include "task/task.hpp"
#include "telemetry/publisher.hpp"
#include "telemetry/telemetry_data.hpp"
#include "telemetry/telemetry_source.hpp"

class TelemetryPublisher : public Publisher {
public:
    TelemetryPublisher(TelemetrySource& p_source, Task& p_task)
        : m_source(p_source)
        , m_task(p_task) {}
    ~TelemetryPublisher() = default;
    TelemetryPublisher(const TelemetryPublisher&) = delete;
    TelemetryPublisher& operator=(const TelemetryPublisher&) = delete;
    TelemetryPublisher(TelemetryPublisher&&) = delete;
    TelemetryPublisher& operator=(TelemetryPublisher&&) = delete;

    void start();

private:
    void loop();
    void handle(const TelemetryEvent& p_event);

    TelemetrySource& m_source;
    Task& m_task;
};

#endif // TELEMETRY_PUBLISHER_HPP
