#ifndef CLAIM_HANDLER_HPP
#define CLAIM_HANDLER_HPP

#include <atomic>

#include "claim/claim_store.hpp"
#include "display/display_controller.hpp"
#include "mqtt/mqtt_bridge.hpp"
#include "task/task.hpp"
#include "utils/binary_semaphore.hpp"
#include "utils/claim_code.hpp"
#include "vendor/freertos.hpp"

// Seeed XIAO Expansion Base user button - wired active-low to GND, needs the internal pull-up.
constexpr int CLAIM_BUTTON_PIN = D1;

// Toggles the device claiming flow (start/stop showing the claim code and notifying the
// server) each time the user button is pressed. Only one instance may exist per program: the
// ISR reaches the task through a single static semaphore handle.
class ClaimHandler {
public:
    ClaimHandler(DisplayController& p_displayController, ClaimStore& p_store, MqttBridge& p_mqttBridge, Task& p_task)
        : m_displayController(p_displayController)
        , m_store(p_store)
        , m_mqttBridge(p_mqttBridge)
        , m_task(p_task) {}
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
    ClaimStore& m_store;
    MqttBridge& m_mqttBridge;
    ClaimCode m_code{};
    std::atomic<bool> m_claimed{false};
    Task& m_task;
    BinarySemaphore m_buttonSignal;

    // The ISR (a plain function pointer, no user data) reaches the task through this.
    static SemaphoreHandle_t s_buttonSignal;
};

#endif // CLAIM_HANDLER_HPP
