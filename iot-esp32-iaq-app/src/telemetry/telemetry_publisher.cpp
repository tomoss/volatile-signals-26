#include "telemetry/telemetry_publisher.hpp"

#include "vendor/arduino.hpp"

#include <cmath>
#include <variant>

void TelemetryPublisher::start() {
    m_task.createAndStart("telemetry_task", [this] {
        loop();
    });
}

void TelemetryPublisher::loop() {
    while (m_task.running()) {
        TelemetryEvent l_event;
        if (m_source.receive(l_event)) {
            handle(l_event);
        }
    }
}

void TelemetryPublisher::handle(const TelemetryEvent& p_event) {
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
