#ifndef HEALTH_REPORTER_HPP
#define HEALTH_REPORTER_HPP

#include "00_vendor/freertos.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "09_utils/task.hpp"

// Periodically publishes device health (RSSI/heap/uptime) over MQTT from its own task.
class HealthReporter {
public:
    HealthReporter(MqttBridge& p_mqttBridge, WifiAdapter& p_wifiAdapter) : m_mqttBridge(p_mqttBridge), m_wifiAdapter(p_wifiAdapter) {}
    ~HealthReporter() = default;
    HealthReporter(const HealthReporter&) = delete;
    HealthReporter& operator=(const HealthReporter&) = delete;
    HealthReporter(HealthReporter&&) = delete;
    HealthReporter& operator=(HealthReporter&&) = delete;

    void start();

private:
    void taskLoop();

    MqttBridge& m_mqttBridge;
    WifiAdapter& m_wifiAdapter;
    Task m_task;
};

#endif // HEALTH_REPORTER_HPP
