#ifndef CLAIM_BUTTON_HANDLER_HPP
#define CLAIM_BUTTON_HANDLER_HPP

#include "00_vendor/freertos.hpp"
#include "02_storage/storage.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "06_display/display_controller.hpp"
#include "09_utils/claim_code_manager.hpp"

// Seeed XIAO Expansion Base user button - wired active-low to GND, needs the internal pull-up.
constexpr int CLAIM_BUTTON_PIN = D1;

// Toggles the device claiming flow (start/stop showing the claim code and notifying the
// server) each time the user button is pressed. Only one instance may exist per program: the
// ISR reaches the task through a single static handle.
class ClaimButtonHandler {
public:
    ClaimButtonHandler(DisplayController& p_displayController, Storage& p_storage, MqttBridge& p_mqttBridge, const ClaimCodeManager& p_claimCodeManager)
        : m_displayController(p_displayController), m_storage(p_storage), m_mqttBridge(p_mqttBridge), m_claimCodeManager(p_claimCodeManager) {}
    ~ClaimButtonHandler();
    ClaimButtonHandler(const ClaimButtonHandler&) = delete;
    ClaimButtonHandler& operator=(const ClaimButtonHandler&) = delete;
    ClaimButtonHandler(ClaimButtonHandler&&) = delete;
    ClaimButtonHandler& operator=(ClaimButtonHandler&&) = delete;

    // Creates the task and wires up the button pin/interrupt.
    void start();

private:
    static void IRAM_ATTR isr();
    static void taskEntry(void* p_parameter);
    void taskLoop();

    DisplayController& m_displayController;
    Storage& m_storage;
    MqttBridge& m_mqttBridge;
    const ClaimCodeManager& m_claimCodeManager;
    TaskHandle_t m_task = nullptr;

    // The ISR (a plain function pointer, no user data) reaches the task through this.
    static TaskHandle_t s_taskHandle;
};

#endif // CLAIM_BUTTON_HANDLER_HPP
