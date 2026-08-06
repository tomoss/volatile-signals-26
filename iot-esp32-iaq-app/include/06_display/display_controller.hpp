#ifndef DISPLAY_CONTROLLER_HPP
#define DISPLAY_CONTROLLER_HPP

#include "06_display/display.hpp"
#include "06_display/display_types.hpp"
#include "07_utils/mutex.hpp"

// Single owner of the Display: every touch of m_display - power toggles and frame draws
// alike - happens under m_mutex, so callers on any task context (button, BLE, WiFi, sensor)
// can drive the panel directly without ever overlapping its I2C traffic with the worker task.
class DisplayController {
public:
    explicit DisplayController(TwoWire& p_wire) : m_display(p_wire) {}
    ~DisplayController();
    DisplayController(const DisplayController&) = delete;
    DisplayController& operator=(const DisplayController&) = delete;
    DisplayController(DisplayController&&) = delete;
    DisplayController& operator=(DisplayController&&) = delete;

    [[nodiscard]] bool init();

    // Thread-safe: safe to call from any task context.
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
    static void taskEntry(void* parameter);

    // p_mutator returns true if it changed any value, in which case the worker task is woken.
    template<typename Mutator>
    void updateState(Mutator p_mutator) {
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
    TaskHandle_t m_task = nullptr;
    Mutex m_mutex;
    bool m_displayEnabled = false;
    DisplayState m_state;
};

#endif // DISPLAY_CONTROLLER_HPP
