#ifndef DISPLAY_CONTROLLER_HPP
#define DISPLAY_CONTROLLER_HPP

#include "06_display/display.hpp"
#include "06_display/display_types.hpp"
#include "09_utils/mutex.hpp"
#include "09_utils/task.hpp"

class DisplayController {
public:
    DisplayController() = default;
    ~DisplayController() = default;
    DisplayController(const DisplayController&) = delete;
    DisplayController& operator=(const DisplayController&) = delete;
    DisplayController(DisplayController&&) = delete;
    DisplayController& operator=(DisplayController&&) = delete;

    bool init(WireWrapper& p_wire);

    // Thread-safe: safe to call from any task context. No-ops if init() failed or wasn't called.
    void enableDisplay();
    void disableDisplay();
    void setWifiStatus(bool p_connected);
    void setMqttStatus(bool p_connected);
    void setEnvironment(uint16_t p_iaq, int8_t p_temperatureC, uint8_t p_accuracy);
    void setProvisioningStatus(uint32_t p_passkey);
    void setClaimingCode(const ClaimCode& p_code);
    void setClaimedStatus(bool p_claimed);
    // Selects which of provision/claiming/already-claimed (if any) takes precedence over the
    // normal WiFi/MQTT/env status screen. Independent of the data setters above.
    void setActiveOverlay(DisplayOverlay p_overlay);

private:
    template<typename Mutator>
    void updateState(Mutator p_mutator) {
        if (!m_available) {
            return;
        }
        {
            const MutexGuard l_guard(m_mutex);
            p_mutator(m_state);
        }
        if (m_enabled) {
            notify();
        }
    }

    void loop();
    void render();
    void notify();
    void wait();

    Display m_display;
    Task m_task;
    Mutex m_mutex;
    bool m_available = false;
    bool m_enabled = false;
    DisplayOverlay m_overlay = DisplayOverlay::None;
    DisplayState m_state;
};

#endif // DISPLAY_CONTROLLER_HPP
