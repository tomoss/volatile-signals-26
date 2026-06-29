#include "04_mqtt/mqtt_bridge.hpp"

#include "00_vendor/arduino.hpp"

MqttBridge::~MqttBridge() {
    if (m_client) {
        esp_mqtt_client_stop(m_client);
        esp_mqtt_client_destroy(m_client);
        m_client = nullptr;
    }
}

bool MqttBridge::init(bool p_enableTls) {
    esp_mqtt_client_config_t config{};
    // TODO: Set up the MQTT client configuration here
    config.broker.address.transport = p_enableTls ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP;

    if (!m_client) {
        m_client = esp_mqtt_client_init(&config);
        if (!m_client) {
            Serial.println("[MQTT] esp_mqtt_client_init failed");
            return false;
        }
    }
    return true;
}