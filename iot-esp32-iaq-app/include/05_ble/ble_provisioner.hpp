#ifndef BLE_PROVISIONER_HPP
#define BLE_PROVISIONER_HPP

#include <array>
#include <functional>

#include "00_vendor/freertos.hpp"
#include "00_vendor/nimble.hpp"
#include "03_wifi/wifi_types.hpp"

class BleProvisioner : private NimBLECharacteristicCallbacks, private NimBLEServerCallbacks {
public:
    using CredentialsCallback = std::function<void(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password)>;

    BleProvisioner() = default;
    ~BleProvisioner();
    BleProvisioner(const BleProvisioner&) = delete;
    const BleProvisioner& operator=(const BleProvisioner&) = delete;
    BleProvisioner(BleProvisioner&&) = delete;
    BleProvisioner& operator=(BleProvisioner&&) = delete;

    void setCredentialsCallback(CredentialsCallback p_callback);

    [[nodiscard]] bool init();

    // Request provisioning to start; the NimBLE init + advertising runs later on the worker
    // task, so this is safe to call from any context (e.g. an event callback).
    void start();

    // Request provisioning to stop; the NimBLE teardown runs later on the worker task, so
    // this is safe to call from any context (e.g. an event callback).
    void stop();

private:
    enum class BleAction : uint8_t { Start = 0, Stop = 1 };

    static void taskEntry(void* parameter);
    void taskLoop();
    void enqueueAction(BleAction action);

    void begin();
    void end();

    void onWrite(NimBLECharacteristic* p_characteristic, NimBLEConnInfo& p_connInfo) override;
    void onConnect(NimBLEServer* p_server, NimBLEConnInfo& p_connInfo) override;
    void onDisconnect(NimBLEServer* p_server, NimBLEConnInfo& p_connInfo, int p_reason) override;
    uint32_t onPassKeyDisplay() override;
    void onAuthenticationComplete(NimBLEConnInfo& p_connInfo) override;

    CredentialsCallback m_callback;
    bool m_running = false;

    NimBLEServer* m_server = nullptr;
    NimBLECharacteristic* m_ssidChar = nullptr;
    NimBLECharacteristic* m_passwordChar = nullptr;

    QueueHandle_t m_queue = nullptr;
    TaskHandle_t m_task = nullptr;

    WifiTypes::Ssid m_ssid{};
    WifiTypes::Password m_password{};
};

#endif // BLE_PROVISIONER_HPP