#include "09_utils/ota_updater.hpp"

#include "00_vendor/arduino.hpp"
#include "00_vendor/http_update.hpp"

#include <algorithm>
#include <cstring>

constexpr uint32_t TASK_STACK_SIZE = 8192;
constexpr UBaseType_t TASK_PRIORITY = 1;

void OtaUpdater::onOtaRequested(std::string_view p_url) {
    if (m_inProgress.exchange(true)) {
        Serial.println("[OTA] Update already in progress, ignoring");
        return;
    }

    Serial.println("[OTA] Preparing for update...");
    m_envSensor.requestModeChange(SensorMode::Disabled);
    m_displayController.disableDisplay();

    const size_t l_len = std::min(p_url.size(), m_url.size() - 1);
    std::memcpy(m_url.data(), p_url.data(), l_len);
    m_url[l_len] = '\0';

    if (xTaskCreate(taskEntry, "ota", TASK_STACK_SIZE, this, TASK_PRIORITY, nullptr) != pdPASS) {
        Serial.println("[OTA] Failed to create OTA task");
        m_envSensor.requestModeChange(SensorMode::LowPower);
        m_displayController.enableDisplay();
        m_inProgress.store(false);
    }
}

void OtaUpdater::taskEntry(void* p_parameter) {
    static_cast<OtaUpdater*>(p_parameter)->runUpdate();
    vTaskDelete(nullptr);
}

void OtaUpdater::runUpdate() {
    WiFiClient l_client;
    Serial.printf("[OTA] Starting update from %s\n", m_url.data());

    const t_httpUpdate_return l_result = httpUpdate.update(l_client, m_url.data());

    switch (l_result) {
    case HTTP_UPDATE_OK:
        Serial.println("[OTA] Update OK, rebooting..."); // httpUpdate reboots automatically on success
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[OTA] No update available");
        break;
    case HTTP_UPDATE_FAILED:
        Serial.printf("[OTA] Failed: %s\n", httpUpdate.getLastErrorString().c_str());
        break;
    }

    if (l_result != HTTP_UPDATE_OK) {
        // Nothing was flashed (or there was no reboot), so undo the pre-update prep in
        // onOtaRequested instead of leaving the sensor disabled and display off.
        m_envSensor.requestModeChange(SensorMode::LowPower);
        m_displayController.enableDisplay();
    }

    m_inProgress.store(false);
}
