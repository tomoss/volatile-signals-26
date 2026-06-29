#ifndef MQTT_BRIDGE_HPP
#define MQTT_BRIDGE_HPP

#include <mqtt_client.h>

#include "02_storage/storage.hpp"

class MqttBridge {
public:
    MqttBridge(Storage& p_storage) : m_storage(p_storage) {}
    ~MqttBridge();

    MqttBridge(const MqttBridge&) = delete;
    MqttBridge& operator=(const MqttBridge&) = delete;
    MqttBridge(MqttBridge&&) = delete;
    MqttBridge& operator=(MqttBridge&&) = delete;

    [[nodiscard]] bool init(const char* p_host = nullptr, uint16_t p_port = 0, bool p_useTls = false);

private:
    esp_mqtt_client_handle_t m_client = nullptr;
    Storage& m_storage;
};

#endif // MQTT_BRIDGE_HPP