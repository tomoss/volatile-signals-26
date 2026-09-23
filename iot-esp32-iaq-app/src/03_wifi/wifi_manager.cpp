#include "03_wifi/wifi_manager.hpp"

#include <cstring>

constexpr uint32_t QUEUE_LENGTH = 10;
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY = 1;

WifiManager::WifiManager(WifiAdapter& p_adapter) : m_adapter(p_adapter), m_sm(m_adapter, m_logger) {}

WifiManager::~WifiManager() {
    if (m_queue != nullptr) {
        vQueueDelete(m_queue);
        m_queue = nullptr;
    }
}

bool WifiManager::init() {
    m_queue = xQueueCreate(QUEUE_LENGTH, sizeof(WifiQueueEvent));

    if (m_queue == nullptr) {
        Serial.println("WiFiManager queue creation failed");
        return false;
    }

    m_adapter.setReconnectTimerCallback([this] {
        postQueueEvent(WifiQueueEventType::Connect);
    });

    m_adapter.setWifiCallback([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            postQueueEvent(WifiQueueEventType::Connected);
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
            postQueueEvent(WifiQueueEventType::Disconnected);
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

    if (!m_task.start("wifi_manager", TASK_STACK_SIZE, TASK_PRIORITY, [this] { taskLoop(); })) {
        return false;
    }

    return true;
}

void WifiManager::start() {
    postQueueEvent(WifiQueueEventType::Start);
}

void WifiManager::stop() {
    postQueueEvent(WifiQueueEventType::Stop);
}

void WifiManager::credentialsUpdated() {
    postQueueEvent(WifiQueueEventType::CredentialsReceived);
}

void WifiManager::taskLoop() {
    for (;;) {
        WifiQueueEvent event;
        if (xQueueReceive(m_queue, &event, portMAX_DELAY) == pdTRUE) {
            handleQueueEvent(event);
        }
    }
}

void WifiManager::handleQueueEvent(const WifiQueueEvent& event) {
    switch (event.type) {
    case WifiQueueEventType::Start:
        m_sm.process_event(EvReqStart{});
        break;

    case WifiQueueEventType::Connect:
        m_sm.process_event(EvReqConnect{});
        break;

    case WifiQueueEventType::Connected:
        m_sm.process_event(EvIsConnected{});
        break;

    case WifiQueueEventType::Disconnect:
        m_sm.process_event(EvReqDisconnect{});
        break;

    case WifiQueueEventType::Reconnect:
        m_sm.process_event(EvReqReconnect{});
        break;

    case WifiQueueEventType::Disconnected:
        m_sm.process_event(EvIsDisconnected{});
        m_sm.process_event(EvReqReconnect{});
        break;

    case WifiQueueEventType::Provisioning:
        m_sm.process_event(EvReqProvisioning{});
        break;

    case WifiQueueEventType::CredentialsReceived:
        m_sm.process_event(EvCredentialsUpdated{});
        break;

    // User can requst just stop, not disconnect
    case WifiQueueEventType::Stop:
        m_sm.process_event(EvReqStop{});
        break;

    default:
        break;
    }
}

void WifiManager::postQueueEvent(WifiQueueEventType type) {
    postQueueEvent(WifiQueueEvent{type});
}

void WifiManager::postQueueEvent(const WifiQueueEvent& event) {
    if (m_queue == nullptr) {
        return;
    }
    xQueueSend(m_queue, &event, 0);
}
