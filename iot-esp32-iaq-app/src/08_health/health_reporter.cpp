#include "08_health/health_reporter.hpp"

#include "00_vendor/arduino.hpp"
#include "08_health/device_health.hpp"

// How often to publish device health (RSSI/heap/uptime) - diagnostic data
constexpr uint32_t PUBLISH_INTERVAL_MS = 60000; // 60 seconds
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY = 1;

HealthReporter::~HealthReporter() {
    if (m_task != nullptr) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
}

void HealthReporter::start() {
    if (pdPASS != xTaskCreate(taskEntry, "health", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task)) {
        Serial.println("HealthReporter task creation failed");
    }
}

void HealthReporter::taskEntry(void* p_parameter) {
    static_cast<HealthReporter*>(p_parameter)->taskLoop();
}

void HealthReporter::taskLoop() {
    for (;;) {
        DeviceHealth l_health;
        l_health.rssi = m_wifiAdapter.getRSSI();
        l_health.heap = ESP.getFreeHeap();
        l_health.minHeap = ESP.getMinFreeHeap();
        l_health.uptime = millis() / 1000;
        l_health.timestamp = time(nullptr);

        m_mqttBridge.sendDeviceHealth(l_health);
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
    }
}
