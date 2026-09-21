#ifndef HEALTH_REPORTER_HPP
#define HEALTH_REPORTER_HPP

#include "00_vendor/freertos.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "04_mqtt/mqtt_bridge.hpp"

// Periodically publishes device health (RSSI/heap/uptime) over MQTT from its own task.
class HealthReporter {
public:
    HealthReporter(MqttBridge& p_mqttBridge, WifiAdapter& p_wifiAdapter) : m_mqttBridge(p_mqttBridge), m_wifiAdapter(p_wifiAdapter) {}
    ~HealthReporter();
    HealthReporter(const HealthReporter&) = delete;
    HealthReporter& operator=(const HealthReporter&) = delete;
    HealthReporter(HealthReporter&&) = delete;
    HealthReporter& operator=(HealthReporter&&) = delete;

    void start();

private:
    static void taskEntry(void* p_parameter);
    void taskLoop();

    MqttBridge& m_mqttBridge;
    WifiAdapter& m_wifiAdapter;
    TaskHandle_t m_task = nullptr;
};

#endif // HEALTH_REPORTER_HPP
