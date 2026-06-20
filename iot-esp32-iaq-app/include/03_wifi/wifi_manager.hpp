#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/sml.hpp"
#include "02_storage/storage.hpp"
#include "wifi_adapter.hpp"
#include "wifi_sm.hpp"

#include <atomic>

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
    bool isConnected() const;

    int getRSSI() const;
    WifiTypes::IpAddr getIPAddress() const;
    WifiTypes::MacAddr getMACAddress() const;
    WifiTypes::Ssid getSSID() const;

private:
    using StateMachine = boost::sml::sm<WifiSm, boost::sml::logger<WifiSmLogger>>;

    static void taskEntry(void* parameter);
    void taskLoop();

    void handleQueueEvent(const WifiQueueEvent& event);

    void postQueueEvent(WifiQueueEventType type);
    void postQueueEvent(const WifiQueueEvent& event);

private:
    WifiAdapter& m_adapter;
    WifiSmLogger m_logger{};
    StateMachine m_sm;

    QueueHandle_t m_queue = nullptr;
    TaskHandle_t m_task = nullptr;

    std::atomic<bool> m_connected{false};
};

#endif // WIFI_MANAGER_HPP