#include "01_sensor/sensor_consumer.hpp"

#include "00_vendor/arduino.hpp"

#include <cmath>
#include <variant>

constexpr uint32_t QUEUE_SIZE = 10;

SensorConsumer::~SensorConsumer() {
    if (m_queue != nullptr) {
        vQueueDelete(m_queue);
        m_queue = nullptr;
    }
}

bool SensorConsumer::init() {
    m_queue = xQueueCreate(QUEUE_SIZE, sizeof(SensorEvent));
    if (m_queue == nullptr) {
        Serial.println("SensorConsumer queue creation failed");
        return false;
    }

    return true;
}

void SensorConsumer::start() {
    m_task.createAndStart("consumer_task", [this] {
        loop();
    });
}

void SensorConsumer::loop() {
    for (;;) {
        SensorEvent l_event;
        if (xQueueReceive(m_queue, &l_event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        handle(l_event);
    }
}

void SensorConsumer::handle(const SensorEvent& p_event) {
    if (const auto* l_mode = std::get_if<SensorMode>(&p_event)) {
        m_mqttBridge.sendSensorInfo(*l_mode);
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

        m_mqttBridge.sendSensorData(l_data);
        m_displayController.setEnvironment(
            static_cast<uint16_t>(std::round(l_data.iaq)), static_cast<int8_t>(std::round(l_data.temp)), static_cast<uint8_t>(l_data.iaqAccuracy));
    }
}
