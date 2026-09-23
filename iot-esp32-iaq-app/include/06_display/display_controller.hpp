#ifndef DISPLAY_CONTROLLER_HPP
#define DISPLAY_CONTROLLER_HPP

#include "06_display/display.hpp"
#include "06_display/display_types.hpp"
#include "09_utils/mutex.hpp"
#include "09_utils/task.hpp"

// Single owner of the Display: every touch of m_display - power toggles and frame draws
// alike - happens under m_mutex, so callers on any task context (button, BLE, WiFi, sensor)
// can drive the panel directly without ever overlapping its I2C traffic with the worker task.
class DisplayController {
public:
    DisplayController() = default;
    ~DisplayController() = default;
    DisplayController(const DisplayController&) = delete;
    DisplayController& operator=(const DisplayController&) = delete;
    DisplayController(DisplayController&&) = delete;
    DisplayController& operator=(DisplayController&&) = delete;

    // Returns false if the display could not be found/initialized. Callers don't need to check
    // the result before using the controller afterwards: every method below becomes a no-op
    // when init() didn't succeed, so there's no "is there a display" branching for callers to do.
    [[nodiscard]] bool init(WireWrapper& p_wire);

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
    // p_mutator returns true if it changed any value, in which case the worker task is woken.
    template<typename Mutator>
    void updateState(Mutator p_mutator) {
        if (!m_available) {
            return;
        }
        bool l_anychanged = false;
        {
            const MutexGuard l_guard(m_mutex);
            l_anychanged = p_mutator(m_state);
        }
        if (m_displayEnabled && l_anychanged) {
            notify();
        }
    }

    void taskLoop();
    void render();
    void notify();
    void wait();

    Display m_display;
    Task m_task;
    Mutex m_mutex;
    bool m_available = false; // True once init() has succeeded; gates every public method.
    bool m_displayEnabled = false;
    DisplayState m_state;
};

#endif // DISPLAY_CONTROLLER_HPP
