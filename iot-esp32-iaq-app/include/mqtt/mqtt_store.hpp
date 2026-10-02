#ifndef MQTT_STORE_HPP
#define MQTT_STORE_HPP

#include <optional>

#include "mqtt/mqtt_types.hpp"

class MqttStore {
public:
    virtual ~MqttStore() = default;

    virtual std::optional<MqttTypes::Host> loadMqttHost() = 0;
    virtual std::optional<MqttTypes::Port> loadMqttPort() = 0;
    virtual std::optional<MqttTypes::Username> loadMqttUsername() = 0;
    virtual std::optional<MqttTypes::Password> loadMqttPassword() = 0;
};

#endif // MQTT_STORE_HPP
