#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/sml.hpp"
#include "02_storage/storage.hpp"
#include "09_utils/task.hpp"
#include "wifi_adapter.hpp"
#include "wifi_sm.hpp"

enum class WifiQueueEventType : uint8_t {
    Start = 0,
    Stop = 1,
    Connect = 2,
    Connected = 3,
    Disconnect = 4,
    Disconnected = 5,
    Reconnect = 6,
    Provisioning = 7,
    CredentialsReceived = 8
};

struct WifiQueueEvent {
    WifiQueueEventType type;
};

class WifiManager {
public:
    WifiManager(WifiAdapter& p_adapter);
    ~WifiManager();
    WifiManager(const WifiManager&) = delete;
    WifiManager& operator=(const WifiManager&) = delete;
    WifiManager(WifiManager&&) = delete;
    WifiManager& operator=(WifiManager&&) = delete;

    [[nodiscard]] bool init();
    void start();
    void stop();
    void credentialsUpdated();

private:
    using StateMachine = boost::sml::sm<WifiSm<WifiAdapter>, boost::sml::logger<WifiSmLogger>>;

    void loop();

    void handleQueueEvent(const WifiQueueEvent& event);
    void postQueueEvent(WifiQueueEventType type);

private:
    WifiAdapter& m_adapter;
    WifiSmLogger m_logger{};
    StateMachine m_sm;

    QueueHandle_t m_queue = nullptr;
    Task m_task;
};

#endif // WIFI_MANAGER_HPP