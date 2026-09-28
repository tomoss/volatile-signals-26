#ifndef OTA_UPDATER_HPP
#define OTA_UPDATER_HPP

#include <atomic>
#include <string_view>

#include "01_sensor/env_sensor.hpp"
#include "04_mqtt/mqtt_types.hpp"
#include "06_display/display_controller.hpp"

// Runs a firmware update in its own task, on request. Meant to be wired into
// MqttBridge::setOnOtaCallback via onOtaRequested(). Only one update may run at a time; a
// request received while one is already in flight is ignored.
class OtaUpdater {
public:
    OtaUpdater(EnvSensor& p_envSensor, DisplayController& p_displayController)
        : m_envSensor(p_envSensor), m_displayController(p_displayController) {}
    ~OtaUpdater() = default;
    OtaUpdater(const OtaUpdater&) = delete;
    OtaUpdater& operator=(const OtaUpdater&) = delete;
    OtaUpdater(OtaUpdater&&) = delete;
    OtaUpdater& operator=(OtaUpdater&&) = delete;

    // Disables the sensor and display, then spawns the task that downloads and flashes p_url.
    // No-op if an update is already in progress.
    void onOtaRequested(std::string_view p_url);

private:
    static void taskEntry(void* p_parameter);
    void runUpdate();

    EnvSensor& m_envSensor;
    DisplayController& m_displayController;
    MqttTypes::Payload m_url{};
    std::atomic<bool> m_inProgress{false};
};

#endif // OTA_UPDATER_HPP
