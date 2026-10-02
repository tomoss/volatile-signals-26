#ifndef CLAIM_HANDLER_HPP
#define CLAIM_HANDLER_HPP

#include <atomic>

#include "display/display_controller.hpp"
#include "mqtt/mqtt_bridge.hpp"
#include "storage/storage.hpp"
#include "utils/claim_code.hpp"
#include "utils/task.hpp"
#include "vendor/freertos.hpp"

// Seeed XIAO Expansion Base user button - wired active-low to GND, needs the internal pull-up.
constexpr int CLAIM_BUTTON_PIN = D1;

// Toggles the device claiming flow (start/stop showing the claim code and notifying the
// server) each time the user button is pressed. Only one instance may exist per program: the
// ISR reaches the task through a single static handle.
class ClaimHandler {
public:
    ClaimHandler(DisplayController& p_displayController, Storage& p_storage, MqttBridge& p_mqttBridge)
        : m_displayController(p_displayController)
        , m_storage(p_storage)
        , m_mqttBridge(p_mqttBridge) {}
    ~ClaimHandler() = default;
    ClaimHandler(const ClaimHandler&) = delete;
    ClaimHandler& operator=(const ClaimHandler&) = delete;
    ClaimHandler(ClaimHandler&&) = delete;
    ClaimHandler& operator=(ClaimHandler&&) = delete;

    // Loads the stored code, or generates + persists a new random one if none exists yet.
    bool init();

    // Creates the task and wires up the button pin/interrupt.
    void start();

    void setClaimed(bool p_claimed);

private:
    static void IRAM_ATTR isr();
    void loop();
    void show();
    void hide();

    DisplayController& m_displayController;
    Storage& m_storage;
    MqttBridge& m_mqttBridge;
    ClaimCode m_code{};
    std::atomic<bool> m_claimed{false};
    Task m_task;

    // The ISR (a plain function pointer, no user data) reaches the task through this.
    static TaskHandle_t s_taskHandle;
};

#endif // CLAIM_HANDLER_HPP
