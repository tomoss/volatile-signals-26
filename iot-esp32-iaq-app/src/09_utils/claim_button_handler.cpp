#include "09_utils/claim_button_handler.hpp"

#include "00_vendor/arduino.hpp"

constexpr uint32_t CLAIM_BUTTON_DEBOUNCE_MS = 200;

TaskHandle_t ClaimButtonHandler::s_taskHandle = nullptr;

// Runs on the interrupt level: debounces in-place (via a static timestamp) and only wakes
// taskLoop on an actual press, so nothing on the button path spins a polling loop.
void IRAM_ATTR ClaimButtonHandler::isr() {
    static uint32_t s_lastIsrMs = 0;
    const uint32_t l_now = millis();
    if (l_now - s_lastIsrMs < CLAIM_BUTTON_DEBOUNCE_MS) {
        return;
    }
    s_lastIsrMs = l_now;

    BaseType_t l_higherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(s_taskHandle, &l_higherPriorityTaskWoken);
    portYIELD_FROM_ISR(l_higherPriorityTaskWoken);
}

void ClaimButtonHandler::start() {
    if (!m_task.createAndStart("claim_button_task", [this] { taskLoop(); })) {
        return;
    }
    s_taskHandle = m_task.handle();

    pinMode(CLAIM_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLAIM_BUTTON_PIN), isr, FALLING);
}

void ClaimButtonHandler::taskLoop() {
    bool l_showing = false;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        l_showing = !l_showing;

        const bool l_claimed = m_storage.loadDeviceClaimStatus();

        if (!l_showing) {
            m_displayController.setActiveOverlay(DisplayOverlay::None);
            if (!l_claimed) {
                m_mqttBridge.clearClaimCode();
                Serial.println("Stopped claiming process on server");
            }
            continue;
        }

        if (l_claimed) {
            Serial.println("Device is already registered");
            m_displayController.setClaimedStatus(true);
            m_displayController.setActiveOverlay(DisplayOverlay::Claim);
            continue;
        }

        m_displayController.setClaimedStatus(false);
        m_displayController.setActiveOverlay(DisplayOverlay::Claim);
        m_mqttBridge.sendClaimCode(m_claimCodeManager.get());
        Serial.println("Started claiming process on server");
        Serial.printf("Claim code: %s\n", m_claimCodeManager.get().data());
    }
}
