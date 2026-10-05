#ifndef FREERTOS_TELEMETRY_QUEUE_HPP
#define FREERTOS_TELEMETRY_QUEUE_HPP

#include <cstddef>

#include "telemetry/telemetry_data.hpp"
#include "telemetry/telemetry_sink.hpp"
#include "telemetry/telemetry_source.hpp"
#include "utils/queue.hpp"
#include "vendor/freertos.hpp"

template<std::size_t N>
class FreeRtosTelemetryQueue : public TelemetrySink, public TelemetrySource {
public:
    FreeRtosTelemetryQueue() = default;
    ~FreeRtosTelemetryQueue() override = default;
    FreeRtosTelemetryQueue(const FreeRtosTelemetryQueue&) = delete;
    FreeRtosTelemetryQueue& operator=(const FreeRtosTelemetryQueue&) = delete;
    FreeRtosTelemetryQueue(FreeRtosTelemetryQueue&&) = delete;
    FreeRtosTelemetryQueue& operator=(FreeRtosTelemetryQueue&&) = delete;

    [[nodiscard]] bool init() { return m_queue.init(); }

    void enqueue(const TelemetryEvent& p_event) override { m_queue.send(p_event); }

    bool receive(TelemetryEvent& p_event) override { return m_queue.receive(p_event); }

private:
    Queue<TelemetryEvent, N> m_queue;
};

#endif // FREERTOS_TELEMETRY_QUEUE_HPP
