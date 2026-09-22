#ifndef SENSOR_REPORTER_HPP
#define SENSOR_REPORTER_HPP

#include "00_vendor/freertos.hpp"
#include "01_sensor/env_sensor.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "06_display/display_controller.hpp"

// Drains EnvSensor's queue from its own task and forwards each reading to MQTT and the
// display, logging every sample to Serial along the way.
class SensorReporter {
public:
    SensorReporter(EnvSensor& p_envSensor, MqttBridge& p_mqttBridge, DisplayController& p_displayController)
        : m_envSensor(p_envSensor), m_mqttBridge(p_mqttBridge), m_displayController(p_displayController) {}
    ~SensorReporter();
    SensorReporter(const SensorReporter&) = delete;
    SensorReporter& operator=(const SensorReporter&) = delete;
    SensorReporter(SensorReporter&&) = delete;
    SensorReporter& operator=(SensorReporter&&) = delete;

    void start();

private:
    static void taskEntry(void* p_parameter);
    void taskLoop();
    void handle(const SensorEvent& p_event);

    EnvSensor& m_envSensor;
    MqttBridge& m_mqttBridge;
    DisplayController& m_displayController;
    TaskHandle_t m_task = nullptr;
};

#endif // SENSOR_REPORTER_HPP
