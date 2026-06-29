#include "04_mqtt/mqtt_bridge.hpp"
#include "00_vendor/arduino.hpp"

#include <esp_crt_bundle.h>

MqttBridge::~MqttBridge() {
    if (m_client) {
        esp_mqtt_client_stop(m_client);
        esp_mqtt_client_destroy(m_client);
        m_client = nullptr;
    }
}

bool MqttBridge::init(bool p_enableTls, int p_reconnectTimeoutMs) {

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
    l_config.session.protocol_ver = MQTT_PROTOCOL_V_5;

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
    l_config.network.reconnect_timeout_ms = p_reconnectTimeoutMs;

    m_client = esp_mqtt_client_init(&l_config);

    if (!m_client) {
        Serial.println("[MQTT] esp_mqtt_client_init failed");
        return false;
    }

    return true;
}

bool MqttBridge::connect() {
    if (m_client == nullptr) {
        Serial.println("MQTT connect failed: call init() first");
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

bool MqttBridge::disconnect() {
    if (m_client == nullptr) {
        Serial.println("MQTT disconnect failed: call init() first");
        return false;
    }

    if (esp_mqtt_client_stop(m_client) != ESP_OK) {
        Serial.println("[MQTT] esp_mqtt_client_stop failed");
        return false;
    }
    return true;
}