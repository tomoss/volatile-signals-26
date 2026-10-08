#ifndef TELEMETRY_PUBLISHER_HPP
#define TELEMETRY_PUBLISHER_HPP

#include <cmath>
#include <variant>

#include "queue/queue.hpp"
#include "task/task.hpp"
#include "telemetry/publisher.hpp"
#include "telemetry/telemetry_data.hpp"
#include "telemetry/telemetry_sink.hpp"
#include "vendor/arduino.hpp"

template<QueueLike<TelemetryEvent> TQueue, TaskLike TTask>
class TelemetryPublisher : public Publisher, public TelemetrySink {
public:
    TelemetryPublisher() = default;
    ~TelemetryPublisher() = default;
    TelemetryPublisher(const TelemetryPublisher&) = delete;
    TelemetryPublisher& operator=(const TelemetryPublisher&) = delete;
    TelemetryPublisher(TelemetryPublisher&&) = delete;
    TelemetryPublisher& operator=(TelemetryPublisher&&) = delete;

    [[nodiscard]] bool init() {
        if (!m_queue.init()) {
            Serial.println("TelemetryPublisher queue creation failed");
            return false;
        }
        return true;
    }

    void start() {
        m_task.createAndStart("telemetry_task", [this] {
            loop();
        });
    }

    void enqueue(const TelemetryEvent& p_event) override { m_queue.send(p_event); }

private:
    void loop() {
        while (m_task.running()) {
            TelemetryEvent l_event;
            if (m_queue.receive(l_event)) {
                handle(l_event);
            }
        }
    }

    void handle(const TelemetryEvent& p_event) {
        if (const auto* l_info = std::get_if<TelemetryInfo>(&p_event)) {
            notify(*l_info);
            return;
        }

        const auto& l_data = std::get<TelemetryData>(p_event);
        Serial.printf("[%lld] "
                      "IAQ=%.1f(acc:%d) "
                      "CO2eq=%.0fppm "
                      "VOCeq=%.2fppm "
                      "T=%.2fC "
                      "RH=%.2f%% "
                      "P=%.2fhPa\n",
                      l_data.timestamp,
                      l_data.iaq,
                      static_cast<int>(l_data.iaqAccuracy),
                      l_data.co2,
                      l_data.voc,
                      l_data.temp,
                      l_data.hum,
                      l_data.pressure);

        if (!std::isnan(l_data.iaq) && !std::isnan(l_data.temp) && !std::isnan(l_data.hum) && !std::isnan(l_data.pressure) && !std::isnan(l_data.co2) &&
            !std::isnan(l_data.voc)) {

            notify(l_data);
        }
    }

    TQueue m_queue;
    TTask m_task;
};

#endif // TELEMETRY_PUBLISHER_HPP
