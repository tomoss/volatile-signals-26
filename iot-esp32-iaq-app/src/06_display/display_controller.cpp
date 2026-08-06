#include "06_display/display_controller.hpp"

constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY = 1;

constexpr std::size_t FIRST_HALF_TEXT_SIZE = 32;
constexpr std::size_t SECOND_HALF_TEXT_SIZE = 16;

DisplayController::~DisplayController() {
    if (m_task != nullptr) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
}

void DisplayController::taskEntry(void* parameter) {
    static_cast<DisplayController*>(parameter)->taskLoop();
}

bool DisplayController::init() {
    if (!m_display.init()) {
        return false;
    }

    if (!m_mutex.init()) {
        return false;
    }

    if (pdPASS != xTaskCreate(taskEntry, "display", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task)) {
        return false;
    }

    return true;
}

void DisplayController::enableDisplay() {
    {
        if (m_displayEnabled) {
            return;
        }
        const MutexGuard l_guard(m_mutex);
        m_displayEnabled = true;
        m_display.setMode(DisplayMode::On);
    }
    notify();
}

void DisplayController::disableDisplay() {
    {
        if (!m_displayEnabled) {
            return;
        }
        const MutexGuard l_guard(m_mutex);
        m_displayEnabled = false;
        m_display.setMode(DisplayMode::Off);
    }
}

void DisplayController::setWifiStatus(bool p_connected) {
    updateState([p_connected](DisplayState& p_outState) {
        if (p_outState.wifiConnected == p_connected) {
            return false; // No change.
        }
        p_outState.wifiConnected = p_connected;
        return true;
    });
}

void DisplayController::setMqttStatus(bool p_connected) {
    updateState([p_connected](DisplayState& p_outState) {
        if (p_outState.mqttConnected == p_connected) {
            return false; // No change.
        }
        p_outState.mqttConnected = p_connected;
        return true;
    });
}

void DisplayController::setEnvironment(uint16_t p_iaq, int8_t p_temperatureC, uint8_t p_accuracy) {
    updateState([p_iaq, p_temperatureC, p_accuracy](DisplayState& p_outState) {
        if (p_outState.iaq == p_iaq && p_outState.temperatureC == p_temperatureC && p_outState.accuracy == p_accuracy) {
            return false; // No change.
        }
        p_outState.iaq = p_iaq;
        p_outState.temperatureC = p_temperatureC;
        p_outState.accuracy = p_accuracy;
        return true;
    });
}

void DisplayController::setProvisioningStatus(uint32_t p_passkey) {
    updateState([p_passkey](DisplayState& p_outState) {
        if (p_outState.provisionPasskey == p_passkey) {
            return false; // No change.
        }
        p_outState.provisionPasskey = p_passkey;
        return true;
    });
}

void DisplayController::setClaimingCode(const ClaimCode& p_code) {
    updateState([p_code](DisplayState& p_outState) {
        if (p_outState.claimCode == p_code) {
            return false; // No change.
        }
        p_outState.claimCode = p_code;
        return true;
    });
}

void DisplayController::setClaimedStatus(bool p_claimed) {
    updateState([p_claimed](DisplayState& p_outState) {
        if (p_outState.claimed == p_claimed) {
            return false; // No change.
        }
        p_outState.claimed = p_claimed;
        return true;
    });
}

void DisplayController::setActiveOverlay(DisplayOverlay p_overlay) {
    updateState([p_overlay](DisplayState& p_outState) {
        if (p_outState.overlay == p_overlay) {
            return false; // No change.
        }
        p_outState.overlay = p_overlay;
        return true;
    });
}

void DisplayController::notify() {
    xTaskNotifyGive(m_task);
}

void DisplayController::wait() {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

void DisplayController::taskLoop() {
    // Initial render
    render();

    for (;;) {
        wait();
        render();
    }
}

void DisplayController::render() {
    const MutexGuard l_guard(m_mutex);

    switch (m_state.overlay) {
    case DisplayOverlay::Provisioning: {
        char l_passkey[SECOND_HALF_TEXT_SIZE];

        if (m_state.provisionPasskey != 0) {
            snprintf(l_passkey, sizeof(l_passkey), "%06lu", static_cast<unsigned long>(m_state.provisionPasskey));
        } else {
            snprintf(l_passkey, sizeof(l_passkey), "------");
        }

        m_display.renderHalves("PROVISIONING", l_passkey);
        return;
    }
    case DisplayOverlay::Claim:
        if (m_state.claimed) {
            m_display.renderHalves("DEVICE", "REGISTERED");
        } else {
            m_display.renderHalves("CLAIM CODE", m_state.claimCode.data());
        }
        return;
    case DisplayOverlay::None: {
        char l_firstHalfDisplay[FIRST_HALF_TEXT_SIZE];
        char l_secondHalfDisplay[SECOND_HALF_TEXT_SIZE];

        // If WiFi is not connected, display "WiFi connecting" on the first half and the IAQ on the second half.
        // If WiFi is connected, display "MQTT connecting" on the first half and the IAQ on the second half.
        if (!m_state.wifiConnected) {
            snprintf(l_firstHalfDisplay, sizeof(l_firstHalfDisplay), "WiFi connecting");
        } else if (!m_state.mqttConnected) {
            snprintf(l_firstHalfDisplay, sizeof(l_firstHalfDisplay), "MQTT connecting");
        } else {
            snprintf(l_firstHalfDisplay,
                     sizeof(l_firstHalfDisplay),
                     "(%d\xc2\xb0"
                     "C) (ACC %u)",
                     m_state.temperatureC,
                     m_state.accuracy);
        }

        snprintf(l_secondHalfDisplay, sizeof(l_secondHalfDisplay), "IAQ: %u", m_state.iaq);
        m_display.renderHalves(l_firstHalfDisplay, l_secondHalfDisplay);
        return;
    }
    }
}
