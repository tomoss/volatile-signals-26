#include "01_sensor/sensor_data_processor.hpp"

#include "00_vendor/arduino.hpp"

#include <cmath>
#include <variant>

void SensorDataProcessor::start() {
    m_task.createAndStart("consumer_task", [this] {
        loop();
    });
}

void SensorDataProcessor::loop() {
    while (m_task.running()) {
        SensorEvent l_event;
        if (m_source.receive(l_event)) {
            handle(l_event);
        }
    }
}

void SensorDataProcessor::handle(const SensorEvent& p_event) {
    if (const auto* l_mode = std::get_if<SensorMode>(&p_event)) {
        notify(*l_mode);
        return;
    }

    const auto& l_data = std::get<SensorData>(p_event);
    Serial.printf("[%lld] "
                  "IAQ=%.1f(acc:%d) "
                  "CO2eq=%.0fppm "
                  "VOCeq=%.2fppm "
                  "Gas=%.0fΩ "
                  "T=%.2fC "
                  "RH=%.2f%% "
                  "RawT=%.2fC "
                  "RawRH=%.2f%% "
                  "P=%.2fhPa\n",
                  l_data.timestamp,
                  l_data.iaq,
                  static_cast<int>(l_data.iaqAccuracy),
                  l_data.co2,
                  l_data.voc,
                  l_data.gas,
                  l_data.temp,
                  l_data.hum,
                  l_data.rawTemp,
                  l_data.rawHum,
                  l_data.pressure);

    if (!std::isnan(l_data.iaq) && !std::isnan(l_data.temp) && !std::isnan(l_data.hum) && !std::isnan(l_data.pressure) && !std::isnan(l_data.co2) &&
        !std::isnan(l_data.voc)) {

        notify(l_data);
    }
}
