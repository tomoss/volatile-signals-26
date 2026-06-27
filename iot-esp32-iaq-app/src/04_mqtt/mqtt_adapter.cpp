#include "04_mqtt/mqtt_adapter.hpp"

#include "00_vendor/arduino.hpp"

MqttAdapter::~MqttAdapter() {
    if (m_client) {
        esp_mqtt_client_stop(m_client);
        esp_mqtt_client_destroy(m_client);
        m_client = nullptr;
    }
}

bool MqttAdapter::init() {
    esp_mqtt_client_config_t config{};
    // TODO: Set up the MQTT client configuration here

    if (!m_client) {
        m_client = esp_mqtt_client_init(&config);
        if (!m_client) {
            Serial.println("[MQTT] esp_mqtt_client_init failed");
            return false;
        }
    }
    return true;
}