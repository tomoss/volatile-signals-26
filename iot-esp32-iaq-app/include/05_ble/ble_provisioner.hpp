#ifndef BLE_PROVISIONER_HPP
#define BLE_PROVISIONER_HPP

#include <array>
#include <functional>

#include "00_vendor/freertos.hpp"
#include "00_vendor/nimble.hpp"
#include "03_wifi/wifi_types.hpp"
#include "09_utils/freertos_task.hpp"
#include "09_utils/queue.hpp"

constexpr std::size_t BLE_ACTION_QUEUE_LENGTH = 2;

class BleProvisioner : private NimBLECharacteristicCallbacks, private NimBLEServerCallbacks {
public:
    using CredentialsCallback = std::function<void(const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password)>;
    using PasskeyDisplayCallback = std::function<void(uint32_t p_passkey)>;

    BleProvisioner() = default;
    ~BleProvisioner() = default;
    BleProvisioner(const BleProvisioner&) = delete;
    const BleProvisioner& operator=(const BleProvisioner&) = delete;
    BleProvisioner(BleProvisioner&&) = delete;
    BleProvisioner& operator=(BleProvisioner&&) = delete;

    void setCredentialsCallback(CredentialsCallback p_callback);
    void setPasskeyDisplayCallback(PasskeyDisplayCallback p_callback);

    [[nodiscard]] bool init();
    void start();

    // Request provisioning to start; the NimBLE init + advertising runs later on the worker
    // task, so this is safe to call from any context (e.g. an event callback).
    void enqueueProvisioningStart();

    // Request provisioning to stop; the NimBLE teardown runs later on the worker task, so
    // this is safe to call from any context (e.g. an event callback).
    void enqueueProvisioningStop();

private:
    enum class BleAction : uint8_t { Start = 0, Stop = 1 };
    using BleActionQueue = Queue<BleAction, BLE_ACTION_QUEUE_LENGTH>;

    void loop();
    void begin();
    void end();

    void onWrite(NimBLECharacteristic* p_characteristic, NimBLEConnInfo& p_connInfo) override;
    void onConnect(NimBLEServer* p_server, NimBLEConnInfo& p_connInfo) override;
    void onDisconnect(NimBLEServer* p_server, NimBLEConnInfo& p_connInfo, int p_reason) override;
    uint32_t onPassKeyDisplay() override;
    void onAuthenticationComplete(NimBLEConnInfo& p_connInfo) override;

    CredentialsCallback m_callback;
    PasskeyDisplayCallback m_passkeyDisplayCallback;
    bool m_running = false;
    // Set by end() when a peer is still connected; deinit is deferred until onDisconnect
    // confirms the link is actually down, instead of tearing down the host stack underneath
    // an active connection.
    bool m_stopPending = false;

    NimBLEServer* m_server = nullptr;
    NimBLECharacteristic* m_ssidChar = nullptr;
    NimBLECharacteristic* m_passwordChar = nullptr;

    BleActionQueue m_queue;
    FreeRtosTask m_task;

    WifiTypes::Ssid m_ssid{};
    WifiTypes::Password m_password{};

    bool m_ssidReceived = false;
    bool m_passwordReceived = false;
};

#endif // BLE_PROVISIONER_HPP