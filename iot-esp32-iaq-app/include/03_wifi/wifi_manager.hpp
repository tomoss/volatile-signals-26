#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/sml.hpp"
#include "02_storage/storage.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "03_wifi/wifi_sm.hpp"
#include "09_utils/queue.hpp"
#include "09_utils/task.hpp"

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

constexpr std::size_t WIFI_EVENT_QUEUE_LENGTH = 10;
class WifiManager {
public:
    WifiManager(WifiAdapter& p_adapter);
    ~WifiManager() = default;
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
    using WifiEventQueue = Queue<WifiQueueEventType, WIFI_EVENT_QUEUE_LENGTH>;

    void loop();

    void handleQueueEvent(WifiQueueEventType type);
    void postQueueEvent(WifiQueueEventType type);

private:
    WifiAdapter& m_adapter;
    WifiSmLogger m_logger{};
    StateMachine m_sm;
    WifiEventQueue m_queue;

    Task m_task;
};

#endif // WIFI_MANAGER_HPP