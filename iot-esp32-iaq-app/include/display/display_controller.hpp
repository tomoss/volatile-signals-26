#ifndef DISPLAY_CONTROLLER_HPP
#define DISPLAY_CONTROLLER_HPP

#include "display/display.hpp"
#include "display/display_types.hpp"
#include "telemetry/consumer.hpp"
#include "utils/mutex.hpp"
#include "utils/task.hpp"

class DisplayController : public TelemetryDataConsumer {
public:
    DisplayController() = default;
    ~DisplayController() = default;
    DisplayController(const DisplayController&) = delete;
    DisplayController& operator=(const DisplayController&) = delete;
    DisplayController(DisplayController&&) = delete;
    DisplayController& operator=(DisplayController&&) = delete;

    bool init(WireWrapper& p_wire);
    void start();

    void update(const TelemetryData& p_data) override;

    // Thread-safe: safe to call from any task context. No-ops if init() failed or wasn't called.
    void enableDisplay();
    void disableDisplay();
    void setWifiStatus(bool p_connected);
    void setMqttStatus(bool p_connected);
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

    void setEnvironment(uint16_t p_iaq, int8_t p_temperatureC, uint8_t p_accuracy);

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
