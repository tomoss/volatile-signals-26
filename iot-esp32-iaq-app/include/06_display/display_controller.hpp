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
    void setWifiStatus(WifiDisplayState p_wifiState);
    void setMqttStatus(MqttDisplayState p_mqttState);
    void setEnvironment(EnvDisplayState p_envState);
    void setProvisioningStatus(ProvisionDisplayState p_provisionState);
    void setClaimingStatus(ClaimingDisplayState p_claimingState);
    void setClaimedStatus(ClaimedDisplayState p_claimedState);

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
