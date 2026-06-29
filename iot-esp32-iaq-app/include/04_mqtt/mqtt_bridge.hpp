#ifndef MQTT_BRIDGE_HPP
#define MQTT_BRIDGE_HPP

#include <mqtt_client.h>

#include "02_storage/storage.hpp"

constexpr int DEFAULT_MQTT_RECONNECT_TIMEOUT_MS = 10000; // 10 seconds

class MqttBridge {
public:
    MqttBridge(Storage& p_storage) : m_storage(p_storage) {}
    ~MqttBridge();

    MqttBridge(const MqttBridge&) = delete;
    MqttBridge& operator=(const MqttBridge&) = delete;
    MqttBridge(MqttBridge&&) = delete;
    MqttBridge& operator=(MqttBridge&&) = delete;

    [[nodiscard]] bool init(bool p_enableTls = false, int p_reconnectTimeoutMs = DEFAULT_MQTT_RECONNECT_TIMEOUT_MS);
    bool connect();
    bool disconnect();

private:
    static void eventHandler(void* p_arg, esp_event_base_t p_base, int32_t p_eventId, void* p_eventData);
    void onEvent(esp_mqtt_event_handle_t p_event);

    esp_mqtt_client_handle_t m_client = nullptr;
    Storage& m_storage;
    bool m_started = false;
};

#endif // MQTT_BRIDGE_HPP