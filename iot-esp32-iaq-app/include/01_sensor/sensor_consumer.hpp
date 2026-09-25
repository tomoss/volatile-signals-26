#ifndef SENSOR_CONSUMER_HPP
#define SENSOR_CONSUMER_HPP

#include "00_vendor/freertos.hpp"
#include "01_sensor/sensor_data.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "06_display/display_controller.hpp"
#include "09_utils/task.hpp"

class SensorConsumer {
public:
    SensorConsumer(MqttBridge& p_mqttBridge, DisplayController& p_displayController)
        : m_mqttBridge(p_mqttBridge)
        , m_displayController(p_displayController) {}
    ~SensorConsumer();
    SensorConsumer(const SensorConsumer&) = delete;
    SensorConsumer& operator=(const SensorConsumer&) = delete;
    SensorConsumer(SensorConsumer&&) = delete;
    SensorConsumer& operator=(SensorConsumer&&) = delete;

    [[nodiscard]] bool init();

    void start();

    QueueHandle_t getQueue() const { return m_queue; }

private:
    void loop();
    void handle(const SensorEvent& p_event);

    MqttBridge& m_mqttBridge;
    DisplayController& m_displayController;
    QueueHandle_t m_queue = nullptr;
    Task m_task;
};

#endif // SENSOR_CONSUMER_HPP
