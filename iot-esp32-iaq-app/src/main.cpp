#include "vendor/arduino.hpp"
#include "vendor/freertos.hpp"

#include "ble/ble_provisioner.hpp"
#include "claim/claim_handler.hpp"
#include "display/display_controller.hpp"
#include "health/health_reporter.hpp"
#include "mqtt/mqtt_bridge.hpp"
#include "queue/freertos_queue.hpp"
#include "sensor/env_sensor.hpp"
#include "storage/storage.hpp"
#include "task/freertos_task.hpp"
#include "telemetry/telemetry_publisher.hpp"
#include "utils/device_info.hpp"
#include "utils/mac_address.hpp"
#include "utils/ota_updater.hpp"
#include "utils/rtc.hpp"
#include "utils/time_sync.hpp"
#include "utils/wire_wrapper.hpp"
#include "wifi/wifi_adapter.hpp"
#include "wifi/wifi_manager.hpp"

#include <esp_system.h>

// Delay duration to wait for board to stabilize
constexpr uint32_t DELAY_UNTIL_STABLE = 2000; // milliseconds

// Delay duration for reboot after failed init
constexpr uint32_t DELAY_UNTIL_RESTART = 6000; // milliseconds

constexpr std::size_t TELEMETRY_EVENT_QUEUE_LENGTH = 10;

/*****************************************************************/
/* Setup                                                         */
/*****************************************************************/
void setup() {
    Serial.begin(115200);
    delay(DELAY_UNTIL_STABLE); // Wait for board to stabilize
    Serial.println("Firmware version: " FIRMWARE_VERSION);

    static WireWrapper wireWrapper;
    static Storage storage;

    static TelemetryPublisher<FreeRtosQueue<TelemetryEvent, TELEMETRY_EVENT_QUEUE_LENGTH>, FreeRtosTask> telemetryPublisher;

    static EnvSensor<FreeRtosTask> envSensor(storage, telemetryPublisher);

    static WifiAdapter wifiAdapter(storage);
    static WifiManager<FreeRtosTask> wifiManager(wifiAdapter);

    static BleProvisioner<FreeRtosTask> bleProvisioner;

    static DisplayController<FreeRtosTask> displayController;

    static MqttBridge mqttBridge(storage, readMacAddress());

    static TimeSync timeSync;
    static RealTimeClock rtc;
    static ClaimHandler<FreeRtosTask> claimHandler(displayController, storage, mqttBridge);
    static HealthReporter<FreeRtosTask> healthReporter(mqttBridge, wifiAdapter);
    static OtaUpdater<FreeRtosTask> otaUpdater(envSensor, displayController);

    telemetryPublisher.addDataConsumer(displayController);
    telemetryPublisher.addDataConsumer(mqttBridge);
    telemetryPublisher.addInfoConsumer(mqttBridge);

    wifiAdapter.setConnectedCallback([] {
        Serial.println("WiFi ConnectedCallback called");
        displayController.setWifiStatus(true);
        if (timeSync.sync()) {
            rtc.write(time(nullptr));
        }
        mqttBridge.connect();
    });

    wifiAdapter.setDisconnectedCallback([] {
        Serial.println("WiFi DisconnectedCallback called");
        displayController.setWifiStatus(false);
    });

    wifiAdapter.setStartProvisioningCallback([] {
        bleProvisioner.enqueueProvisioningStart();
        displayController.setProvisioningStatus(0);
        displayController.setActiveOverlay(DisplayOverlay::Provisioning);
    });

    wifiAdapter.setStopProvisioningCallback([] {
        bleProvisioner.enqueueProvisioningStop();
        displayController.setActiveOverlay(DisplayOverlay::None);
    });

    bleProvisioner.setPasskeyDisplayCallback([](uint32_t p_passkey) {
        Serial.printf("[BLE] Pairing passkey: %06lu\n", p_passkey);
        displayController.setProvisioningStatus(p_passkey);
    });

    bleProvisioner.setCredentialsCallback([](const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password) {
        wifiManager.saveCredentialsAndEnqueueUpdate(p_ssid, p_password);
    });

    static const DeviceInfo deviceInfo = collectDeviceInfo();

    mqttBridge.setOnConnectedCallback([] {
        displayController.setMqttStatus(true);
        mqttBridge.sendDeviceInfo(deviceInfo);
    });

    mqttBridge.setOnDisconnectedCallback([] {
        displayController.setMqttStatus(false);
    });

    mqttBridge.setOnOtaCallback([](std::string_view p_url) {
        otaUpdater.onOtaRequested(p_url);
    });

    mqttBridge.setOnSensorModeCallback([](SensorMode p_mode) {
        envSensor.enqueueModeChange(p_mode);
    });

    mqttBridge.setOnClaimStatusCallback([](bool p_claimed) {
        claimHandler.setClaimed(p_claimed);
    });

    // Mandatory modules initialization
    const bool l_initOk = telemetryPublisher.init() && wireWrapper.init() && storage.init() && envSensor.init(wireWrapper) && wifiManager.init() &&
                          bleProvisioner.init() && mqttBridge.init(true);
    if (!l_initOk) {
        Serial.println("Mandatory module init failed, restarting the board...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    // Not mandatory, so not required to succeed
    rtc.init(wireWrapper);
    displayController.init(wireWrapper);
    claimHandler.init();

    rtc.seedSystemClock();

    displayController.start();
    claimHandler.start();
    telemetryPublisher.start();
    envSensor.start();
    wifiManager.start();
    bleProvisioner.start();
    healthReporter.start();

    displayController.enableDisplay();
    wifiManager.enqueueWifiStart();

    vTaskDelete(nullptr);
}

void loop() {}
