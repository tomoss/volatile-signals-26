#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "storage/storage.hpp"
#include "utils/queue.hpp"
#include "utils/task.hpp"
#include "vendor/arduino.hpp"
#include "vendor/sml.hpp"
#include "wifi/wifi_adapter.hpp"
#include "wifi/wifi_sm.hpp"

enum class WifiQueueEvent : uint8_t {
    StartRequested = 0,
    StopRequested = 1,
    ConnectRequested = 2,
    Connected = 3,
    DisconnectRequested = 4,
    Disconnected = 5,
    ReconnectRequested = 6,
    ProvisioningRequested = 7,
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
    void enqueueWifiStart();
    void enqueueWifiStop();
    void saveCredentialsAndEnqueueUpdate(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password);

private:
    using StateMachine = boost::sml::sm<WifiSm<WifiAdapter>, boost::sml::logger<WifiSmLogger>>;
    using WifiEventQueue = Queue<WifiQueueEvent, WIFI_EVENT_QUEUE_LENGTH>;

    void loop();

    void handleQueueEvent(WifiQueueEvent type);
    void enqueueEvent(WifiQueueEvent type);

private:
    WifiAdapter& m_adapter;
    WifiSmLogger m_logger{};
    StateMachine m_sm;
    WifiEventQueue m_queue;

    Task m_task;
};

#endif // WIFI_MANAGER_HPP