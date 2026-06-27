#ifndef MQTT_ADAPTER_HPP
#define MQTT_ADAPTER_HPP

#include <mqtt_client.h>

class MqttAdapter {
public:
    MqttAdapter() = default;
    ~MqttAdapter();

    MqttAdapter(const MqttAdapter&) = delete;
    MqttAdapter& operator=(const MqttAdapter&) = delete;
    MqttAdapter(MqttAdapter&&) = delete;
    MqttAdapter& operator=(MqttAdapter&&) = delete;

    bool init();

private:
    esp_mqtt_client_handle_t m_client = nullptr;
};

#endif // MQTT_ADAPTER_HPP