#include "claim/claim_handler.hpp"

#include "task/freertos_task.hpp"
#include "vendor/arduino.hpp"

#include <cstdio>
#include <esp_random.h>

constexpr uint32_t CLAIM_BUTTON_DEBOUNCE_MS = 50;

// The ISR (a plain function pointer, no user data) reaches the task through this.
static SemaphoreHandle_t s_buttonSignal = nullptr;

static void IRAM_ATTR claimButtonIsr() {
    BaseType_t l_higherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(s_buttonSignal, &l_higherPriorityTaskWoken);
    portYIELD_FROM_ISR(l_higherPriorityTaskWoken);
}

template<TaskLike TTask>
bool ClaimHandler<TTask>::init() {
    if (!m_buttonSignal.init()) {
        Serial.println("Claim button signal init failed");
        return false;
    }

    m_claimed.store(m_store.loadDeviceClaimStatus());

    if (const auto l_saved = m_store.loadClaimCode()) {
        m_code = *l_saved;
        m_displayController.setClaimingCode(m_code);
        return true;
    }

    const uint32_t l_random = esp_random() % 1000000;
    snprintf(m_code.data(), m_code.size(), "%06lu", static_cast<unsigned long>(l_random));
    if (!m_store.saveClaimCode(m_code)) {
        Serial.println("Failed to save claim code");
        return false;
    }

    m_displayController.setClaimingCode(m_code);
    return true;
}

template<TaskLike TTask>
void ClaimHandler<TTask>::setClaimed(bool p_claimed) {
    if (m_claimed.exchange(p_claimed) != p_claimed) {
        m_store.saveDeviceClaimStatus(p_claimed);
    }
    m_displayController.setClaimedStatus(p_claimed);
}

template<TaskLike TTask>
void ClaimHandler<TTask>::start() {
    if (m_buttonSignal.handle() == nullptr) {
        return;
    }

    if (!m_task.createAndStart("claim_button_task", [this] {
            loop();
        })) {
        return;
    }
    s_buttonSignal = m_buttonSignal.handle();

    pinMode(CLAIM_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLAIM_BUTTON_PIN), claimButtonIsr, FALLING);
}

template<TaskLike TTask>
void ClaimHandler<TTask>::loop() {
    bool l_showing = false;

    while (m_task.running()) {
        m_buttonSignal.take();

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
        m_buttonSignal.take(0);
    }
}

template<TaskLike TTask>
void ClaimHandler<TTask>::show() {
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

template<TaskLike TTask>
void ClaimHandler<TTask>::hide() {
    m_displayController.setActiveOverlay(DisplayOverlay::None);
    m_mqttBridge.clearClaimCode();
    Serial.println("Stopped claiming process on server");
}

template class ClaimHandler<FreeRtosTask>;
