#include "wifi/wifi_manager.hpp"

#include <cstring>

constexpr uint32_t QUEUE_LENGTH = 10;

WifiManager::WifiManager(WifiAdapter& p_adapter, Task& p_task) : m_adapter(p_adapter), m_sm(m_adapter, m_logger), m_task(p_task) {}

bool WifiManager::init() {

    if (!m_queue.init()) {
        Serial.println("WiFiManager queue creation failed");
        return false;
    }

    m_adapter.setReconnectCallback([this] {
        enqueueEvent(WifiQueueEvent::ConnectRequested);
    });

    m_adapter.setWifiCallback([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            enqueueEvent(WifiQueueEvent::Connected);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
            const auto reason = info.wifi_sta_disconnected.reason;
            // ASSOC_FAIL (203): spurious reset fired by WiFi.begin() before the actual attempt.
            // CONNECTION_FAIL (205): stack rejected esp_wifi_connect() — already in bad state,
            // calling it again would INT_WDT the chip.
            // ASSOC_LEAVE (8) is NOT filtered: it can mean a genuine AP-initiated
            // disconnect, not just our own cleanup — the SM tolerates a stale reconnect
            // timer firing later (see wifi_sm.hpp), so it's safe to always react to it.
            // These two never reach the SM, so log them here — they'd otherwise be invisible.
            if (reason == WIFI_REASON_ASSOC_FAIL || reason == WIFI_REASON_CONNECTION_FAIL) {
                Serial.printf("WiFi disconnected (reason=%d, filtered out)\n", reason);
                break;
            }
            Serial.printf("WiFi disconnected (reason=%d)\n", reason);
            enqueueEvent(WifiQueueEvent::Disconnected);
            break;
        }

        default:
            break;
        }
    });

    if (!m_adapter.init()) {
        Serial.println("WiFiAdapter init failed");
        return false;
    }

    return true;
}

void WifiManager::start() {
    m_task.createAndStart("wifi_task", [this] {
        loop();
    });
}

void WifiManager::enqueueWifiStart() {
    enqueueEvent(WifiQueueEvent::StartRequested);
}

void WifiManager::enqueueWifiStop() {
    enqueueEvent(WifiQueueEvent::StopRequested);
}

void WifiManager::saveCredentialsAndEnqueueUpdate(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password) {
    if (m_adapter.saveCredentials(p_ssid, p_password)) {
        enqueueEvent(WifiQueueEvent::CredentialsReceived);
    }
}

void WifiManager::loop() {
    while (m_task.running()) {
        WifiQueueEvent type;
        if (m_queue.receive(type)) {
            handleQueueEvent(type);
        }
    }
}

void WifiManager::handleQueueEvent(WifiQueueEvent type) {
    switch (type) {
    case WifiQueueEvent::StartRequested:
        m_sm.process_event(EvStartRequested{});
        break;

    case WifiQueueEvent::ConnectRequested:
        m_sm.process_event(EvConnectRequested{});
        break;

    case WifiQueueEvent::Connected:
        m_sm.process_event(EvConnected{});
        break;

    case WifiQueueEvent::DisconnectRequested:
        m_sm.process_event(EvDisconnectRequested{});
        break;

    case WifiQueueEvent::ReconnectRequested:
        m_sm.process_event(EvReconnectRequested{});
        break;

    case WifiQueueEvent::Disconnected:
        m_sm.process_event(EvDisconnected{});
        m_sm.process_event(EvReconnectRequested{});
        break;

    case WifiQueueEvent::ProvisioningRequested:
        m_sm.process_event(EvProvisioningRequested{});
        break;

    case WifiQueueEvent::CredentialsReceived:
        m_sm.process_event(EvCredentialsUpdated{});
        break;

    // User can requst just stop, not disconnect
    case WifiQueueEvent::StopRequested:
        m_sm.process_event(EvStopRequested{});
        break;

    default:
        break;
    }
}

void WifiManager::enqueueEvent(WifiQueueEvent type) {
    m_queue.send(type);
}
