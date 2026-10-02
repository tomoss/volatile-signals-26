#ifndef HEALTH_REPORTER_HPP
#define HEALTH_REPORTER_HPP

#include "vendor/freertos.hpp"
#include "wifi/wifi_adapter.hpp"
#include "mqtt/mqtt_bridge.hpp"
#include "utils/task.hpp"

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
