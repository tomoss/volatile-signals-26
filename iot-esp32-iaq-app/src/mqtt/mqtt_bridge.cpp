#include "mqtt/mqtt_bridge.hpp"
#include "mqtt/mqtt_types.hpp"
#include "vendor/arduino.hpp"
#include "vendor/arduinojson.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string_view>

#include <esp_crt_bundle.h>

constexpr int DEFAULT_MQTT_RECONNECT_TIMEOUT_MS = 10000; // 10 seconds
constexpr int DEFAULT_MQTT_PUB_QOS = 1;                  // QoS level 1
constexpr int DEFAULT_MQTT_SUB_QOS = 1;                  // QoS level 1

void MqttBridge::buildTopic(MqttTypes::Topic& p_topic, std::string_view p_suffix) const {
    snprintf(p_topic.data(), p_topic.size(), "iaq/%s/%.*s", m_mac.data(), static_cast<int>(p_suffix.size()), p_suffix.data());
}

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

    auto l_host = m_store.loadMqttHost();
    if (!l_host) {
        Serial.println("[MQTT] Failed to load MQTT host from storage");
        return false;
    }

    auto l_port = m_store.loadMqttPort();
    if (!l_port) {
        Serial.println("[MQTT] Failed to load MQTT port from storage");
        return false;
    }

    auto l_username = m_store.loadMqttUsername();
    if (!l_username) {
        Serial.println("[MQTT] Failed to load MQTT username from storage");
        return false;
    }

    auto l_password = m_store.loadMqttPassword();
    if (!l_password) {
        Serial.println("[MQTT] Failed to load MQTT password from storage");
        return false;
    }

    esp_mqtt_client_config_t l_config{};

    // *** SESSION CONFIGURATION ***
    l_config.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    l_config.session.disable_clean_session = false;
    l_config.session.keepalive = 30;

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
    l_config.session.last_will.topic = m_deviceStatusPubTopic.data();
    l_config.session.last_will.msg = "offline";
    l_config.session.last_will.msg_len = 0;
    l_config.session.last_will.qos = DEFAULT_MQTT_PUB_QOS;
    l_config.session.last_will.retain = 1;

    // *** TOPICS CREATION ***

    buildTopic(m_telemetryDataPubTopic, "sensor_data");
    buildTopic(m_deviceHealthPubTopic, "device_health");
    buildTopic(m_deviceInfoPubTopic, "device_info");
    buildTopic(m_telemetryInfoPubTopic, "sensor_info");
    buildTopic(m_deviceStatusPubTopic, "device_status");
    buildTopic(m_claimRequestPubTopic, "claim_request");
    buildTopic(m_commandSubTopic, "command");
    buildTopic(m_sensorSubTopic, "sensor");
    buildTopic(m_claimStatusSubTopic, "claim_status");
    buildTopic(m_otaSubTopic, "ota");

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

void MqttBridge::update(const TelemetryData& p_data) {
    sendTelemetryData(p_data);
}

void MqttBridge::update(const TelemetryInfo& p_info) {
    sendTelemetryInfo(p_info);
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

void MqttBridge::sendTelemetryData(const TelemetryData& p_data) {
    JsonDocument l_doc;
    l_doc["iaq"] = p_data.iaq;
    l_doc["iaq_accuracy"] = p_data.iaqAccuracy;
    l_doc["co2"] = p_data.co2;
    l_doc["voc"] = p_data.voc;
    l_doc["temp"] = p_data.temp;
    l_doc["hum"] = p_data.hum;
    l_doc["pressure"] = p_data.pressure;
    l_doc["timestamp"] = static_cast<long long>(p_data.timestamp);

    MqttTypes::Payload l_payload{};
    if (measureJson(l_doc) >= l_payload.size()) {
        Serial.println("[MQTT] Telemetry data payload too large");
        return;
    }

    const size_t l_payloadLen = serializeJson(l_doc, l_payload.data(), l_payload.size());
    publish(m_telemetryDataPubTopic, l_payload.data(), static_cast<int>(l_payloadLen));
}

void MqttBridge::sendDeviceHealth(const DeviceHealth& p_health) {
    // If not connected, no need to send device health data
    if (!m_connected.load()) {
        return;
    }

    JsonDocument l_doc;
    l_doc["rssi"] = p_health.rssi;
    l_doc["heap"] = p_health.heap;
    l_doc["min_heap"] = p_health.minHeap;
    l_doc["uptime"] = p_health.uptime;
    l_doc["timestamp"] = static_cast<long long>(p_health.timestamp);

    MqttTypes::Payload l_payload{};
    if (measureJson(l_doc) >= l_payload.size()) {
        Serial.println("[MQTT] Device health payload too large");
        return;
    }

    const size_t l_payloadLen = serializeJson(l_doc, l_payload.data(), l_payload.size());
    publish(m_deviceHealthPubTopic, l_payload.data(), static_cast<int>(l_payloadLen));
}

void MqttBridge::sendDeviceInfo(const DeviceInfo& p_info) {
    // If not connected, no need to send device info; it'll be resent on the next successful connect.
    if (!m_connected.load()) {
        return;
    }

    JsonDocument l_doc;
    l_doc["firmware_version"] = p_info.firmwareVersion;
    l_doc["chip_model"] = p_info.chipModel;
    l_doc["chip_revision"] = p_info.chipRevision;
    l_doc["chip_cores"] = p_info.chipCores;
    l_doc["reset_reason"] = p_info.resetReason;
    l_doc["total_heap"] = p_info.totalHeap;

    MqttTypes::Payload l_payload{};
    if (measureJson(l_doc) >= l_payload.size()) {
        Serial.println("[MQTT] Device info payload too large");
        return;
    }

    const size_t l_payloadLen = serializeJson(l_doc, l_payload.data(), l_payload.size());
    // Retained so the broker hands the last-known info to any late-subscribing client
    // (e.g. the backend restarting) without waiting for the device's next reconnect.
    publish(m_deviceInfoPubTopic, l_payload.data(), static_cast<int>(l_payloadLen), 1);
}

void MqttBridge::sendTelemetryInfo(const TelemetryInfo& p_info) {
    JsonDocument l_doc;
    l_doc["mode"] = p_info.sensorMode;

    MqttTypes::Payload l_payload{};
    if (measureJson(l_doc) >= l_payload.size()) {
        Serial.println("[MQTT] Telemetry info payload too large");
        return;
    }

    const size_t l_payloadLen = serializeJson(l_doc, l_payload.data(), l_payload.size());
    // Retained so a late-subscribing client immediately learns the current mode instead of
    // waiting for it to change again.
    publish(m_telemetryInfoPubTopic, l_payload.data(), static_cast<int>(l_payloadLen), 1);
}

void MqttBridge::sendClaimCode(const ClaimCode& p_code) {
    // If not connected, there's nothing to do; the code isn't queued or retried, the user just
    // presses the button again.
    if (!m_connected.load()) {
        return;
    }

    JsonDocument l_doc;
    l_doc["code"] = std::string_view(p_code.data(), strnlen(p_code.data(), p_code.size()));

    MqttTypes::Payload l_payload{};
    if (measureJson(l_doc) >= l_payload.size()) {
        Serial.println("[MQTT] Claim code payload too large");
        return;
    }

    const size_t l_payloadLen = serializeJson(l_doc, l_payload.data(), l_payload.size());
    publish(m_claimRequestPubTopic, l_payload.data(), static_cast<int>(l_payloadLen));
}

void MqttBridge::clearClaimCode() {
    if (!m_connected.load()) {
        return;
    }

    JsonDocument l_doc;
    l_doc["code"] = "";

    MqttTypes::Payload l_payload{};
    const size_t l_payloadLen = serializeJson(l_doc, l_payload.data(), l_payload.size());
    publish(m_claimRequestPubTopic, l_payload.data(), static_cast<int>(l_payloadLen));
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
        handleMessage(l_topic, l_payload);
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

void MqttBridge::handleMessage(std::string_view p_topic, std::string_view p_payload) {
    if (p_topic == m_commandSubTopic.data()) {
        handleCommandMessage(p_payload);
    } else if (p_topic == m_sensorSubTopic.data()) {
        handleSensorMessage(p_payload);
    } else if (p_topic == m_claimStatusSubTopic.data()) {
        handleClaimStatusMessage(p_payload);
    } else if (p_topic == m_otaSubTopic.data()) {
        handleOtaMessage(p_payload);
    }
}

void MqttBridge::handleCommandMessage(std::string_view p_payload) {
    JsonDocument l_doc;
    if (deserializeJson(l_doc, p_payload.data(), p_payload.size())) {
        Serial.println("[MQTT] Invalid command payload");
        return;
    }

    if (l_doc["device"] == "reboot") {
        Serial.println("Rebooting...");
        Serial.flush();
        esp_restart();
        return;
    }

    Serial.println("[MQTT] Unknown command");
}

void MqttBridge::handleSensorMessage(std::string_view p_payload) {
    JsonDocument l_doc;
    if (deserializeJson(l_doc, p_payload.data(), p_payload.size())) {
        Serial.println("[MQTT] Invalid sensor payload");
        return;
    }

    if (!m_onSensorModeCallback) {
        return;
    }

    if (l_doc["mode"] == "lp") {
        m_onSensorModeCallback(SensorMode::LowPower);
    } else if (l_doc["mode"] == "ulp") {
        m_onSensorModeCallback(SensorMode::UltraLowPower);
    } else {
        Serial.println("[MQTT] Unknown sensor mode");
    }
}

void MqttBridge::handleClaimStatusMessage(std::string_view p_payload) {
    JsonDocument l_doc;
    if (deserializeJson(l_doc, p_payload.data(), p_payload.size())) {
        Serial.println("[MQTT] Invalid claim status payload");
        return;
    }

    const JsonVariant l_status = l_doc["status"];
    if (!l_status.is<bool>()) {
        Serial.println("[MQTT] Unknown claim status");
        return;
    }

    if (m_onClaimStatusCallback) {
        m_onClaimStatusCallback(l_status.as<bool>());
    }
}

void MqttBridge::handleOtaMessage(std::string_view p_payload) {
    JsonDocument l_doc;
    if (deserializeJson(l_doc, p_payload.data(), p_payload.size())) {
        Serial.println("[MQTT] Invalid OTA payload");
        return;
    }

    const JsonVariant l_url = l_doc["url"];
    if (!l_url.is<const char*>()) {
        Serial.println("[MQTT] Missing OTA url");
        return;
    }

    if (m_onOtaCallback) {
        m_onOtaCallback(l_url.as<const char*>());
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
    publish(m_deviceStatusPubTopic, l_offlinePayload, static_cast<int>(sizeof(l_offlinePayload) - 1), 1);

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
    subscribe(m_sensorSubTopic.data(), DEFAULT_MQTT_SUB_QOS);
    subscribe(m_claimStatusSubTopic.data(), DEFAULT_MQTT_SUB_QOS);
    subscribe(m_otaSubTopic.data(), DEFAULT_MQTT_SUB_QOS);

    constexpr char l_onlinePayload[] = "online";
    publish(m_deviceStatusPubTopic, l_onlinePayload, static_cast<int>(sizeof(l_onlinePayload) - 1), 1);

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