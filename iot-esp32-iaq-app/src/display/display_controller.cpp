#include "display/display_controller.hpp"

#include "task/freertos_task.hpp"

#include <cmath>

constexpr std::size_t FIRST_HALF_TEXT_SIZE = 32;
constexpr std::size_t SECOND_HALF_TEXT_SIZE = 16;

template<TaskLike TTask>
bool DisplayController<TTask>::init(WireWrapper& p_wire) {
    if (!m_display.init(p_wire)) {
        Serial.println("Display init failed (continuing without display)");
        return false;
    }

    if (!m_mutex.init()) {
        Serial.println("Display mutex init failed (continuing without display)");
        return false;
    }

    if (!m_renderSignal.init()) {
        Serial.println("Display render signal init failed (continuing without display)");
        return false;
    }

    m_available = true;
    return true;
}

template<TaskLike TTask>
void DisplayController<TTask>::start() {
    if (!m_available) {
        return;
    }

    m_task.createAndStart("display_task", [this] {
        loop();
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::update(const TelemetryData& p_data) {
    setEnvironment(static_cast<uint16_t>(std::round(p_data.iaq)), static_cast<int8_t>(std::round(p_data.temp)), p_data.iaqAccuracy);
}

template<TaskLike TTask>
void DisplayController<TTask>::enableDisplay() {
    if (!m_available) {
        return;
    }
    {
        const MutexGuard l_guard(m_mutex);
        if (m_enabled) {
            return;
        }
        m_enabled = true;
        m_display.setMode(DisplayMode::On);
    }
    notify();
}

template<TaskLike TTask>
void DisplayController<TTask>::disableDisplay() {
    if (!m_available) {
        return;
    }
    const MutexGuard l_guard(m_mutex);
    if (!m_enabled) {
        return;
    }
    m_enabled = false;
    m_display.setMode(DisplayMode::Off);
}

template<TaskLike TTask>
void DisplayController<TTask>::setWifiStatus(bool p_connected) {
    updateState([p_connected](DisplayState& p_outState) {
        p_outState.wifiConnected = p_connected;
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::setMqttStatus(bool p_connected) {
    updateState([p_connected](DisplayState& p_outState) {
        p_outState.mqttConnected = p_connected;
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::setEnvironment(uint16_t p_iaq, int8_t p_temperatureC, uint8_t p_accuracy) {
    updateState([p_iaq, p_temperatureC, p_accuracy](DisplayState& p_outState) {
        p_outState.iaq = p_iaq;
        p_outState.temperatureC = p_temperatureC;
        p_outState.accuracy = p_accuracy;
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::setProvisioningStatus(uint32_t p_passkey) {
    updateState([p_passkey](DisplayState& p_outState) {
        p_outState.provisionPasskey = p_passkey;
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::setClaimingCode(const ClaimCode& p_code) {
    updateState([p_code](DisplayState& p_outState) {
        p_outState.claimCode = p_code;
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::setClaimedStatus(bool p_claimed) {
    updateState([p_claimed](DisplayState& p_outState) {
        p_outState.claimed = p_claimed;
    });
}

template<TaskLike TTask>
void DisplayController<TTask>::setActiveOverlay(DisplayOverlay p_overlay) {
    if (!m_available) {
        return;
    }
    {
        const MutexGuard l_guard(m_mutex);
        m_overlay = p_overlay;
    }
    if (m_enabled) {
        notify();
    }
}

template<TaskLike TTask>
void DisplayController<TTask>::notify() {
    m_renderSignal.give();
}

template<TaskLike TTask>
void DisplayController<TTask>::wait() {
    m_renderSignal.take();
}

template<TaskLike TTask>
void DisplayController<TTask>::loop() {
    // Initial render
    render();

    while (m_task.running()) {
        wait();
        render();
    }
}

template<TaskLike TTask>
void DisplayController<TTask>::render() {
    const MutexGuard l_guard(m_mutex);

    switch (m_overlay) {
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

template class DisplayController<FreeRtosTask>;
