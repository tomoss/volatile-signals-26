#ifndef TELEMETRY_PUBLISHER_HPP
#define TELEMETRY_PUBLISHER_HPP

#include "telemetry/publisher.hpp"
#include "telemetry/telemetry_data.hpp"
#include "telemetry/telemetry_sink.hpp"
#include "utils/queue.hpp"
#include "utils/task.hpp"
#include "vendor/freertos.hpp"

constexpr std::size_t TELEMETRY_EVENT_QUEUE_LENGTH = 10;

class TelemetryPublisher : public Publisher, public TelemetrySink {
public:
    TelemetryPublisher() {};
    ~TelemetryPublisher() = default;
    TelemetryPublisher(const TelemetryPublisher&) = delete;
    TelemetryPublisher& operator=(const TelemetryPublisher&) = delete;
    TelemetryPublisher(TelemetryPublisher&&) = delete;
    TelemetryPublisher& operator=(TelemetryPublisher&&) = delete;

    [[nodiscard]] bool init();

    void start();

    void enqueue(const TelemetryEvent& p_event) override;

private:
    using TelemetryEventQueue = Queue<TelemetryEvent, TELEMETRY_EVENT_QUEUE_LENGTH>;
    void loop();
    void handle(const TelemetryEvent& p_event);

    TelemetryEventQueue m_queue;
    Task m_task;
};

#endif // TELEMETRY_PUBLISHER_HPP
