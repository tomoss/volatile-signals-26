#ifndef SENSOR_CONSUMER_HPP
#define SENSOR_CONSUMER_HPP

#include "00_vendor/freertos.hpp"
#include "01_sensor/sensor_types.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "06_display/display_controller.hpp"
#include "09_utils/queue.hpp"
#include "09_utils/task.hpp"

constexpr std::size_t SENSOR_EVENT_QUEUE_LENGTH = 10;

class SensorConsumer {
public:
    SensorConsumer(MqttBridge& p_mqttBridge, DisplayController& p_displayController)
        : m_mqttBridge(p_mqttBridge)
        , m_displayController(p_displayController) {}
    ~SensorConsumer() = default;
    SensorConsumer(const SensorConsumer&) = delete;
    SensorConsumer& operator=(const SensorConsumer&) = delete;
    SensorConsumer(SensorConsumer&&) = delete;
    SensorConsumer& operator=(SensorConsumer&&) = delete;

    [[nodiscard]] bool init();

    void start();

    void enqueueSensorEvent(const SensorEvent& p_event);

private:
    using SensorEventQueue = Queue<SensorEvent, SENSOR_EVENT_QUEUE_LENGTH>;
    void loop();
    void handle(const SensorEvent& p_event);

    MqttBridge& m_mqttBridge;
    DisplayController& m_displayController;
    SensorEventQueue m_queue;
    Task m_task;
};

#endif // SENSOR_CONSUMER_HPP
