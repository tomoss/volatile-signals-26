#ifndef MQTT_BRIDGE_HPP
#define MQTT_BRIDGE_HPP

#include <functional>

#include <mqtt_client.h>

#include "01_sensor/sensor_data.hpp"
#include "02_storage/storage.hpp"
#include "04_mqtt/mqtt_types.hpp"

class MqttBridge {
public:
    using OnConnectedCallback = std::function<void()>;
    using OnDisconnectedCallback = std::function<void()>;

    MqttBridge(Storage& p_storage) : m_storage(p_storage) {}
    ~MqttBridge();

    MqttBridge(const MqttBridge&) = delete;
    MqttBridge& operator=(const MqttBridge&) = delete;
    MqttBridge(MqttBridge&&) = delete;
    MqttBridge& operator=(MqttBridge&&) = delete;

    [[nodiscard]] bool init(bool p_enableTls = false);
    bool connect();
    bool disconnect();

    void setOnConnectedCallback(OnConnectedCallback p_callback) { m_onConnectedCallback = std::move(p_callback); }
    void setOnDisconnectedCallback(OnDisconnectedCallback p_callback) { m_onDisconnectedCallback = std::move(p_callback); }

    void sendSensorData(const SensorData& p_data);

private:
    static void eventHandler(void* p_arg, esp_event_base_t p_base, int32_t p_eventId, void* p_eventData);
    void onEvent(esp_mqtt_event_handle_t p_event);

    void handleConnected();
    void handleDisconnected();

    esp_mqtt_client_handle_t m_client = nullptr;
    Storage& m_storage;
    MqttTypes::Topic m_sensorPubtopic{};
    bool m_started = false;
    OnConnectedCallback m_onConnectedCallback;
    OnDisconnectedCallback m_onDisconnectedCallback;
};

#endif // MQTT_BRIDGE_HPP