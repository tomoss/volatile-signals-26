#ifndef MQTT_BRIDGE_HPP
#define MQTT_BRIDGE_HPP

#include <atomic>
#include <functional>
#include <string_view>

#include <mqtt_client.h>

#include "01_sensor/sensor_data.hpp"
#include "01_sensor/sensor_types.hpp"
#include "02_storage/storage.hpp"
#include "04_mqtt/mqtt_types.hpp"
#include "07_utils/device_health.hpp"
#include "07_utils/device_info.hpp"

class MqttBridge {
public:
    using OnConnectedCallback = std::function<void()>;
    using OnDisconnectedCallback = std::function<void()>;
    using OnCommandCallback = std::function<void(std::string_view p_data)>;
    using OnOtaCallback = std::function<void(std::string_view p_url)>;

    MqttBridge(Storage& p_storage) : m_storage(p_storage) {}
    ~MqttBridge();

    MqttBridge(const MqttBridge&) = delete;
    MqttBridge& operator=(const MqttBridge&) = delete;
    MqttBridge(MqttBridge&&) = delete;
    MqttBridge& operator=(MqttBridge&&) = delete;

    [[nodiscard]] bool init(bool p_enableTls = false);

    // Currently only called from wifi callback, so no need to be thread-safe. If called from multiple threads, make it thread-safe.
    bool connect();
    bool disconnect();

    void setOnConnectedCallback(OnConnectedCallback p_callback) { m_onConnectedCallback = std::move(p_callback); }
    void setOnDisconnectedCallback(OnDisconnectedCallback p_callback) { m_onDisconnectedCallback = std::move(p_callback); }
    void setOnCommandCallback(OnCommandCallback p_callback) { m_onCommandCallback = std::move(p_callback); }
    void setOnOtaCallback(OnOtaCallback p_callback) { m_onOtaCallback = std::move(p_callback); }

    // Will be sent also if not connected, messages will be queued into the outbox and sent when connected.
    void sendSensorData(const SensorData& p_data);
    // Will be sent only if connected, otherwise ignored. If not connected, no need to send device health data.
    void sendDeviceHealth(const DeviceHealth& p_health);
    // Static device metadata, sent retained once per connection instead of on the health timer.
    void sendDeviceInfo(const DeviceInfo& p_info);
    // Sent retained whenever the sensor's mode actually changes, so a late subscriber immediately
    // learns the current mode instead of waiting for the next reading.
    void sendSensorInfo(SensorMode p_mode);

private:
    void publish(const MqttTypes::Topic& p_topic, const char* p_data, int p_len, int p_retain = 0);
    void subscribe(const char* p_topic, int p_qos = 0);

    static void eventHandler(void* p_arg, esp_event_base_t p_base, int32_t p_eventId, void* p_eventData);
    void onEvent(esp_mqtt_event_handle_t p_event);

    void handleConnected(bool p_sessionPresent);
    void handleDisconnected();

    esp_mqtt_client_handle_t m_client = nullptr;
    Storage& m_storage;
    MqttTypes::Topic m_sensorDataPubTopic{};
    MqttTypes::Topic m_deviceHealthPubTopic{};
    MqttTypes::Topic m_deviceInfoPubTopic{};
    MqttTypes::Topic m_sensorInfoPubTopic{};
    MqttTypes::Topic m_deviceStatusPubTopic{};
    MqttTypes::Topic m_commandSubTopic{};
    MqttTypes::Topic m_otaSubTopic{};
    bool m_started = false;
    std::atomic<bool> m_connected{false};
    OnConnectedCallback m_onConnectedCallback;
    OnDisconnectedCallback m_onDisconnectedCallback;
    OnCommandCallback m_onCommandCallback;
    OnOtaCallback m_onOtaCallback;
};

#endif // MQTT_BRIDGE_HPP