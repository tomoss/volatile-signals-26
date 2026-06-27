#ifndef MQTT_ADAPTER_HPP
#define MQTT_ADAPTER_HPP

#include <mqtt_client.h>

class MqttAdapter {
private:
    esp_mqtt_client_handle_t m_client = nullptr;
};

#endif // MQTT_ADAPTER_HPP