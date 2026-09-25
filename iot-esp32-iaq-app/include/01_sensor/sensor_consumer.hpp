#ifndef SENSOR_CONSUMER_HPP
#define SENSOR_CONSUMER_HPP

#include "00_vendor/freertos.hpp"
#include "01_sensor/sensor_types.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "06_display/display_controller.hpp"
#include "09_utils/queue.hpp"
#include "09_utils/task.hpp"

class SensorConsumer {
public:
    SensorConsumer(MqttBridge& p_mqttBridge, DisplayController& p_displayController, SensorEventQueue& p_queue)
        : m_mqttBridge(p_mqttBridge)
        , m_displayController(p_displayController)
        , m_queue(p_queue) {}
    ~SensorConsumer() = default;
    SensorConsumer(const SensorConsumer&) = delete;
    SensorConsumer& operator=(const SensorConsumer&) = delete;
    SensorConsumer(SensorConsumer&&) = delete;
    SensorConsumer& operator=(SensorConsumer&&) = delete;

    void start();

private:
    void loop();
    void handle(const SensorEvent& p_event);

    MqttBridge& m_mqttBridge;
    DisplayController& m_displayController;
    SensorEventQueue& m_queue;
    Task m_task;
};

#endif // SENSOR_CONSUMER_HPP
