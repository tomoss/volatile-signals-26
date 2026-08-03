#include "04_mqtt/mqtt_bridge.hpp"
#include "00_vendor/arduino.hpp"
#include "04_mqtt/mqtt_types.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

#include <esp_crt_bundle.h>
#include <esp_mac.h>

constexpr int DEFAULT_MQTT_RECONNECT_TIMEOUT_MS = 10000; // 10 seconds
constexpr int DEFAULT_MQTT_PUB_QOS = 1;                  // QoS level 1
constexpr int DEFAULT_MQTT_SUB_QOS = 1;                  // QoS level 1

MqttBridge::~MqttBridge() {
    if (m_client) {
        esp_mqtt_client_stop(m_client);
        esp_mqtt_client_destroy(m_client);
        m_client = nullptr;
    }
}

bool MqttBridge::init(bool p_enableTls) {

    if (m_client) {
        Serial.println("[MQTT] Client already initialized");
        return false;
    }

    auto l_host = m_storage.loadMqttHost();
    if (!l_host) {
        Serial.println("[MQTT] Failed to load MQTT host from storage");
        return false;
    }

    auto l_port = m_storage.loadMqttPort();
    if (!l_port) {
        Serial.println("[MQTT] Failed to load MQTT port from storage");
        return false;
    }

    auto l_username = m_storage.loadMqttUsername();
    if (!l_username) {
        Serial.println("[MQTT] Failed to load MQTT username from storage");
        return false;
    }

    auto l_password = m_storage.loadMqttPassword();
    if (!l_password) {
        Serial.println("[MQTT] Failed to load MQTT password from storage");
        return false;
    }

    esp_mqtt_client_config_t l_config{};

    // *** SESSION CONFIGURATION ***
    l_config.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    // Disable clean session to allow the broker to store subscriptions and undelivered messages for the client
    l_config.session.disable_clean_session = true;

    // *** CREDENTIALS CONFIGURATION ***
    l_config.credentials.username = l_username.value().data();
    l_config.credentials.authentication.password = l_password.value().data();

    // Default client id is ESP32_%CHIPID% where %CHIPID% are last 3 bytes of MAC address in hex format
    // No need to set l_config.credentials.client_id
    l_config.credentials.client_id = nullptr;

    // *** BROKER CONFIGURATION ***
    l_config.broker.address.hostname = l_host.value().data();
    l_config.broker.address.port = l_port.value();

    l_config.broker.address.transport = p_enableTls ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP;

    if (p_enableTls) {
        l_config.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    }

    // *** NETWORK CONFIGURATION ***
    l_config.network.reconnect_timeout_ms = DEFAULT_MQTT_RECONNECT_TIMEOUT_MS;

    // *** LAST WILL AND TESTAMENT CONFIGURATION ***
    // LWT: the broker publishes this retained message on our behalf if it detects an
    // ungraceful disconnect (crash, power loss, WiFi drop) instead of a clean MQTT disconnect.
    // msg_len 0 tells esp-mqtt to derive the length from the NULL-terminated msg string.
    l_config.session.last_will.topic = m_statusPubTopic.data();
    l_config.session.last_will.msg = "offline";
    l_config.session.last_will.msg_len = 0;
    l_config.session.last_will.qos = DEFAULT_MQTT_PUB_QOS;
    l_config.session.last_will.retain = 1;

    // *** TOPICS CREATION ***

    // esp_read_mac() reads the factory-burned MAC from eFuse directly, so it's valid
    // immediately at boot - unlike WiFi.macAddress(), it doesn't need the STA netif to be up.
    std::array<uint8_t, 6> l_mac{};
    esp_read_mac(l_mac.data(), ESP_MAC_WIFI_STA);

    snprintf(m_sensorPubtopic.data(),
             m_sensorPubtopic.size(),
             "iaq/%02X:%02X:%02X:%02X:%02X:%02X/sensor",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);

    snprintf(m_healthPubTopic.data(),
             m_healthPubTopic.size(),
             "iaq/%02X:%02X:%02X:%02X:%02X:%02X/health",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);

    snprintf(m_infoPubTopic.data(),
             m_infoPubTopic.size(),
             "iaq/%02X:%02X:%02X:%02X:%02X:%02X/info",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);

    snprintf(m_statusPubTopic.data(),
             m_statusPubTopic.size(),
             "iaq/%02X:%02X:%02X:%02X:%02X:%02X/status",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);

    snprintf(m_commandSubTopic.data(),
             m_commandSubTopic.size(),
             "iaq/%02X:%02X:%02X:%02X:%02X:%02X/command",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);

    snprintf(m_otaSubTopic.data(),
             m_otaSubTopic.size(),
             "iaq/%02X:%02X:%02X:%02X:%02X:%02X/ota",
             l_mac[0],
             l_mac[1],
             l_mac[2],
             l_mac[3],
             l_mac[4],
             l_mac[5]);

    m_client = esp_mqtt_client_init(&l_config);

    if (!m_client) {
        Serial.println("[MQTT] esp_mqtt_client_init failed");
        return false;
    }

    if (esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, eventHandler, this) != ESP_OK) {
        Serial.println("[MQTT] esp_mqtt_client_register_event failed");
        return false;
    }

    return true;
}

bool MqttBridge::connect() {
    if (m_client == nullptr) {
        Serial.println("[MQTT] connect failed: call init() first");
        return false;
    }

    if (!m_started) {
        if (esp_mqtt_client_start(m_client) != ESP_OK) {
            Serial.println("[MQTT] esp_mqtt_client_start failed");
            return false;
        }
        m_started = true;
        return true;
    }

    if (m_started) {
        if (esp_mqtt_client_reconnect(m_client) != ESP_OK) {
            Serial.println("[MQTT] esp_mqtt_client_reconnect failed");
            return false;
        }
        return true;
    }

    return false;
}

void MqttBridge::sendSensorData(const SensorData& p_data) {
    MqttTypes::Payload l_payload{};
    const int l_len =
        snprintf(l_payload.data(),
                 l_payload.size(),
                 "{\"iaq\":%.2f,\"iaq_accuracy\":%d,\"co2\":%.2f,\"voc\":%.2f,\"temp\":%.2f,\"hum\":%.2f,\"pressure\":%.2f,\"timestamp\":%lld}",
                 p_data.iaq,
                 static_cast<int>(p_data.iaqAccuracy),
                 p_data.co2,
                 p_data.voc,
                 p_data.temp,
                 p_data.hum,
                 p_data.pressure,
                 static_cast<long long>(p_data.timestamp));

    if (l_len < 0) {
        Serial.println("[MQTT] Failed to serialize sensor data");
        return;
    }

    // snprintf returns the length it would have written even if truncated, so clamp to what
    // actually fits in the buffer (size() - 1, since one byte is reserved for the null terminator).
    const size_t l_payloadLen = std::min(static_cast<size_t>(l_len), l_payload.size() - 1);
    publish(m_sensorPubtopic, l_payload.data(), static_cast<int>(l_payloadLen));
}

void MqttBridge::sendDeviceHealth(const DeviceHealth& p_health) {
    // If not connected, no need to send device health data
    if (!m_connected.load()) {
        return;
    }

    MqttTypes::Payload l_payload{};
    const int l_len = snprintf(l_payload.data(),
                               l_payload.size(),
                               "{\"rssi\":%d,\"heap\":%lu,\"min_heap\":%lu,\"uptime\":%lu,\"timestamp\":%lld}",
                               p_health.rssi,
                               static_cast<unsigned long>(p_health.heap),
                               static_cast<unsigned long>(p_health.minHeap),
                               static_cast<unsigned long>(p_health.uptime),
                               static_cast<long long>(p_health.timestamp));

    if (l_len < 0) {
        Serial.println("[MQTT] Failed to serialize device health");
        return;
    }

    const size_t l_payloadLen = std::min(static_cast<size_t>(l_len), l_payload.size() - 1);
    publish(m_healthPubTopic, l_payload.data(), static_cast<int>(l_payloadLen));
}

void MqttBridge::sendDeviceInfo(const DeviceInfo& p_info) {
    // If not connected, no need to send device info; it'll be resent on the next successful connect.
    if (!m_connected.load()) {
        return;
    }

    MqttTypes::Payload l_payload{};
    const int l_len = snprintf(l_payload.data(),
                               l_payload.size(),
                               "{\"firmware_version\":\"%s\",\"chip_model\":\"%s\",\"chip_revision\":%u,\"chip_cores\":%u,"
                               "\"reset_reason\":%u,\"total_heap\":%lu}",
                               p_info.firmwareVersion,
                               p_info.chipModel,
                               static_cast<unsigned int>(p_info.chipRevision),
                               static_cast<unsigned int>(p_info.chipCores),
                               static_cast<unsigned int>(p_info.resetReason),
                               static_cast<unsigned long>(p_info.totalHeap));

    if (l_len < 0) {
        Serial.println("[MQTT] Failed to serialize device info");
        return;
    }

    const size_t l_payloadLen = std::min(static_cast<size_t>(l_len), l_payload.size() - 1);
    // Retained so the broker hands the last-known info to any late-subscribing client
    // (e.g. the backend restarting) without waiting for the device's next reconnect.
    publish(m_infoPubTopic, l_payload.data(), static_cast<int>(l_payloadLen), 1);
}

void MqttBridge::publish(const MqttTypes::Topic& p_topic, const char* p_data, int p_len, int p_retain) {
    if (m_client == nullptr) {
        Serial.println("[MQTT] Publish failed: call init() first");
        return;
    }

    int l_result = esp_mqtt_client_publish(m_client, p_topic.data(), p_data, p_len, DEFAULT_MQTT_PUB_QOS, p_retain);

    if (l_result >= 0) {
        Serial.printf("[MQTT] Published to %s, size: %d\n", p_topic.data(), p_len);
        return;
    }

    if (l_result == -1) {
        Serial.printf("[MQTT] Failed to publish to %s\n", p_topic.data());
        return;
    }

    if (l_result == -2) {
        Serial.printf("[MQTT] Failed to publish to %s: Full outbox\n", p_topic.data());
        return;
    }
}

void MqttBridge::subscribe(const char* p_topic, int p_qos) {
    if (m_client == nullptr) {
        Serial.println("[MQTT] Subscribe failed: call init() first");
        return;
    }

    if (p_topic == nullptr) {
        Serial.println("[MQTT] Subscribe failed: topic is null");
        return;
    }

    int l_result = esp_mqtt_client_subscribe(m_client, p_topic, p_qos);

    if (l_result < 0) {
        Serial.printf("[MQTT] Failed to subscribe to %s\n", p_topic);
    } else {
        Serial.printf("[MQTT] Subscribed to %s, msg_id=%d\n", p_topic, l_result);
    }
}

void MqttBridge::eventHandler(void* p_arg, esp_event_base_t /*p_base*/, int32_t /*p_eventId*/, void* p_eventData) {
    static_cast<MqttBridge*>(p_arg)->onEvent(static_cast<esp_mqtt_event_handle_t>(p_eventData));
}

void MqttBridge::onEvent(esp_mqtt_event_handle_t p_event) {
    switch (p_event->event_id) {

    case MQTT_EVENT_CONNECTED:
        Serial.printf("[MQTT] Connected (session_present=%d)\n", p_event->session_present);
        handleConnected(p_event->session_present);
        break;

    case MQTT_EVENT_DATA: {
        const std::string_view l_topic{p_event->topic, static_cast<size_t>(p_event->topic_len)};
        const std::string_view l_payload{p_event->data, static_cast<size_t>(p_event->data_len)};
        if (l_topic == m_commandSubTopic.data() && m_onCommandCallback) {
            m_onCommandCallback(l_payload);
        } else if (l_topic == m_otaSubTopic.data() && m_onOtaCallback) {
            m_onOtaCallback(l_payload);
        }
        break;
    }

    case MQTT_EVENT_DISCONNECTED:
        Serial.println("[MQTT] Disconnected");
        handleDisconnected();
        break;

    case MQTT_EVENT_ERROR:

        if (p_event->error_handle == nullptr) {
            Serial.println("[MQTT] Error event (no error_handle)");
            break;
        }

        switch (p_event->error_handle->error_type) {

        case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
            Serial.printf("[MQTT] Connection refused, return_code=%d\n", static_cast<int>(p_event->error_handle->connect_return_code));
            break;

        case MQTT_ERROR_TYPE_SUBSCRIBE_FAILED:
            Serial.printf("[MQTT] Subscribe failed, msg_id=%d\n", p_event->msg_id);
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}

bool MqttBridge::disconnect() {
    if (m_client == nullptr) {
        Serial.println("[MQTT] disconnect failed: call init() first");
        return false;
    }

    // A clean MQTT disconnect does NOT trigger the broker's LWT (that only fires on an
    // ungraceful connection loss), so publish "offline" ourselves before stopping.
    constexpr char l_offlinePayload[] = "offline";
    publish(m_statusPubTopic, l_offlinePayload, static_cast<int>(sizeof(l_offlinePayload) - 1), 1);

    // Unlike esp_mqtt_client_stop(), esp_mqtt_client_disconnect() blocks until the DISCONNECT
    // packet (and anything still queued ahead of it, like the offline publish above) has
    // actually been sent, so the broker sees a graceful close instead of a dropped connection.
    if (esp_mqtt_client_disconnect(m_client) != ESP_OK) {
        Serial.println("[MQTT] esp_mqtt_client_disconnect failed");
        return false;
    }

    if (esp_mqtt_client_stop(m_client) != ESP_OK) {
        Serial.println("[MQTT] esp_mqtt_client_stop failed");
        return false;
    }

    Serial.println("[MQTT] Disconnected cleanly");
    return true;
}

void MqttBridge::handleConnected(bool /*p_sessionPresent*/) {
    m_connected.store(true);
    subscribe(m_commandSubTopic.data(), DEFAULT_MQTT_SUB_QOS);
    subscribe(m_otaSubTopic.data(), DEFAULT_MQTT_SUB_QOS);

    constexpr char l_onlinePayload[] = "online";
    publish(m_statusPubTopic, l_onlinePayload, static_cast<int>(sizeof(l_onlinePayload) - 1), 1);

    if (m_onConnectedCallback) {
        m_onConnectedCallback();
    }
}

void MqttBridge::handleDisconnected() {
    m_connected.store(false);
    if (m_onDisconnectedCallback) {
        m_onDisconnectedCallback();
    }
}