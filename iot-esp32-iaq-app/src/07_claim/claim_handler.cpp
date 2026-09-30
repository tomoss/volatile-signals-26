#include "07_claim/claim_handler.hpp"

#include "00_vendor/arduino.hpp"

#include <cstdio>
#include <esp_random.h>

constexpr uint32_t CLAIM_BUTTON_DEBOUNCE_MS = 50;

TaskHandle_t ClaimHandler::s_taskHandle = nullptr;

void IRAM_ATTR ClaimHandler::isr() {
    BaseType_t l_higherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(s_taskHandle, &l_higherPriorityTaskWoken);
    portYIELD_FROM_ISR(l_higherPriorityTaskWoken);
}

bool ClaimHandler::init() {
    m_claimed.store(m_storage.loadDeviceClaimStatus());

    if (const auto l_saved = m_storage.loadClaimCode()) {
        m_code = *l_saved;
        m_displayController.setClaimingCode(m_code);
        return true;
    }

    const uint32_t l_random = esp_random() % 1000000;
    snprintf(m_code.data(), m_code.size(), "%06lu", static_cast<unsigned long>(l_random));
    if (!m_storage.saveClaimCode(m_code)) {
        Serial.println("Failed to save claim code");
        return false;
    }

    m_displayController.setClaimingCode(m_code);
    return true;
}

void ClaimHandler::setClaimed(bool p_claimed) {
    if (m_claimed.exchange(p_claimed) != p_claimed) {
        m_storage.saveDeviceClaimStatus(p_claimed);
    }
    m_displayController.setClaimedStatus(p_claimed);
}

void ClaimHandler::start() {
    if (!m_task.createAndStart("claim_button_task", [this] {
            loop();
        })) {
        return;
    }
    s_taskHandle = m_task.handle();

    pinMode(CLAIM_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLAIM_BUTTON_PIN), isr, FALLING);
}

void ClaimHandler::loop() {
    bool l_showing = false;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        l_showing = !l_showing;
        if (l_showing) {
            show();
        } else {
            hide();
        }

        while (digitalRead(CLAIM_BUTTON_PIN) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(CLAIM_BUTTON_DEBOUNCE_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(CLAIM_BUTTON_DEBOUNCE_MS));
        ulTaskNotifyTake(pdTRUE, 0);
    }
}

void ClaimHandler::show() {
    const bool l_claimed = m_claimed.load();
    m_displayController.setClaimedStatus(l_claimed);
    m_displayController.setActiveOverlay(DisplayOverlay::Claim);

    if (l_claimed) {
        Serial.println("Device is already registered");
        return;
    }

    m_mqttBridge.sendClaimCode(m_code);
    Serial.println("Started claiming process on server");
    Serial.printf("Claim code: %s\n", m_code.data());
}

void ClaimHandler::hide() {
    m_displayController.setActiveOverlay(DisplayOverlay::None);
    m_mqttBridge.clearClaimCode();
    Serial.println("Stopped claiming process on server");
}
