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

void DisplayController::setWifiStatus(WifiDisplayState p_wifiState) {
    updateState([p_wifiState](DisplayState& p_outState) {
        if (p_outState.wifi == p_wifiState) {
            return false; // No change.
        }
        p_outState.wifi = p_wifiState;
        return true;
    });
}

void DisplayController::setMqttStatus(MqttDisplayState p_mqttState) {
    updateState([p_mqttState](DisplayState& p_outState) {
        if (p_outState.mqtt == p_mqttState) {
            return false; // No change.
        }
        p_outState.mqtt = p_mqttState;
        return true;
    });
}

void DisplayController::setEnvironment(EnvDisplayState p_envState) {
    updateState([p_envState](DisplayState& p_outState) {
        if (p_outState.env == p_envState) {
            return false; // No change.
        }
        p_outState.env = p_envState;
        return true;
    });
}

void DisplayController::setProvisioningStatus(ProvisionDisplayState p_provisionState) {
    updateState([p_provisionState](DisplayState& p_outState) {
        if (p_outState.provision == p_provisionState) {
            return false; // No change.
        }
        p_outState.provision = p_provisionState;
        return true;
    });
}

void DisplayController::setClaimedStatus(ClaimingDisplayState p_claimingState) {
    updateState([p_claimingState](DisplayState& p_outState) {
        if (p_outState.claiming == p_claimingState) {
            return false; // No change.
        }
        p_outState.claiming = p_claimingState;
        return true;
    });
}

void DisplayController::setClaimedStatus(AlreadyClaimedDisplayState p_alreadyClaimedState) {
    updateState([p_alreadyClaimedState](DisplayState& p_outState) {
        if (p_outState.alreadyClaimed == p_alreadyClaimedState) {
            return false; // No change.
        }
        p_outState.alreadyClaimed = p_alreadyClaimedState;
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

    // If provisioning is active, it takes precedence over the other display states.
    if (m_state.provision.active) {
        char l_passkey[SECOND_HALF_TEXT_SIZE];

        if (m_state.provision.passkey != 0) {
            snprintf(l_passkey, sizeof(l_passkey), "%06lu", static_cast<unsigned long>(m_state.provision.passkey));
        } else {
            snprintf(l_passkey, sizeof(l_passkey), "------");
        }

        m_display.renderHalves("PROVISIONING", l_passkey);
        return;
    }

    // Take precedence over WiFi/MQTT/env status, but not over active provisioning.
    if (m_state.claiming.active) {
        m_display.renderHalves("CLAIM CODE", m_state.claiming.code.data());
        return;
    }

    if (m_state.alreadyClaimed.active) {
        m_display.renderHalves("DEVICE", "REGISTERED");
        return;
    }

    char l_firstHalfDisplay[FIRST_HALF_TEXT_SIZE];
    char l_secondHalfDisplay[SECOND_HALF_TEXT_SIZE];

    // If provisioning is not active, display the WiFi status.
    // If WiFi is not connected, display "WiFi connecting" on the first half and the IAQ on the second half.
    // If WiFi is connected, display "MQTT connecting" on the first half and the IAQ on the second half.
    if (!m_state.wifi.connected) {
        snprintf(l_firstHalfDisplay, sizeof(l_firstHalfDisplay), "WiFi connecting");
    } else if (!m_state.mqtt.connected) {
        snprintf(l_firstHalfDisplay, sizeof(l_firstHalfDisplay), "MQTT connecting");
    } else {
        snprintf(l_firstHalfDisplay,
                 sizeof(l_firstHalfDisplay),
                 "(%d\xc2\xb0"
                 "C) (ACC %u)",
                 m_state.env.temperatureC,
                 m_state.env.accuracy);
    }

    snprintf(l_secondHalfDisplay, sizeof(l_secondHalfDisplay), "IAQ: %u", m_state.env.iaq);
    m_display.renderHalves(l_firstHalfDisplay, l_secondHalfDisplay);
}
