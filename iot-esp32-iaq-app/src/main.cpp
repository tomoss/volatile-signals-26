#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

#include "01_sensor/env_sensor.hpp"
#include "01_sensor/sensor_consumer.hpp"
#include "02_storage/storage.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "03_wifi/wifi_manager.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "05_ble/ble_provisioner.hpp"
#include "06_display/display_controller.hpp"
#include "07_command/command_handler.hpp"
#include "08_health/health_reporter.hpp"
#include "09_utils/claim_button_handler.hpp"
#include "09_utils/claim_code_manager.hpp"
#include "09_utils/device_info.hpp"
#include "09_utils/ota_updater.hpp"
#include "09_utils/rtc.hpp"
#include "09_utils/time_sync.hpp"
#include "09_utils/wire_wrapper.hpp"

#include <esp_system.h>

// Delay duration to wait for board to stabilize
constexpr uint32_t DELAY_UNTIL_STABLE = 2000; // milliseconds

// Delay duration for reboot after failed init
constexpr uint32_t DELAY_UNTIL_RESTART = 6000; // milliseconds

/*****************************************************************/
/* Setup                                                         */
/*****************************************************************/
void setup() {
    Serial.begin(115200);
    delay(DELAY_UNTIL_STABLE); // Wait for board to stabilize
    Serial.println("Firmware version: " FIRMWARE_VERSION);

    static WireWrapper wireWrapper;
    static Storage storage;
    static EnvSensor envSensor(storage);
    static WifiAdapter wifiAdapter(storage);
    static WifiManager wifiManager(wifiAdapter);
    static BleProvisioner bleProvisioner;
    static DisplayController displayController;
    static MqttBridge mqttBridge(storage);
    static TimeSync timeSync;
    static RealTimeClock rtc;
    static ClaimCodeManager claimCodeManager(storage);
    static CommandHandler commandHandler(envSensor, mqttBridge, storage, claimCodeManager, displayController);
    static HealthReporter healthReporter(mqttBridge, wifiAdapter);
    static ClaimButtonHandler claimButtonHandler(displayController, storage, mqttBridge, claimCodeManager);
    static SensorConsumer sensorConsumer(mqttBridge, displayController);
    static OtaUpdater otaUpdater(envSensor, displayController);

    // Mandatory modules initialization
    if (!wireWrapper.init() || !storage.init() || !sensorConsumer.init() || !envSensor.init(wireWrapper) || !wifiManager.init() ||
        !bleProvisioner.init() || !mqttBridge.init(true) || !claimCodeManager.init() || !commandHandler.init()) {
        Serial.println("Mandatory module init failed, restarting the board...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    envSensor.setConsumerQueue(sensorConsumer.getQueue());

    // Not mandatory, so not required to succeed
    rtc.init(wireWrapper);
    rtc.seedSystemClock();
    displayController.init(wireWrapper);
    displayController.enableDisplay();
    displayController.setClaimingCode(claimCodeManager.get());

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
        bleProvisioner.start();
        displayController.setProvisioningStatus(0);
        displayController.setActiveOverlay(DisplayOverlay::Provisioning);
    });

    wifiAdapter.setStopProvisioningCallback([] {
        bleProvisioner.stop();
        displayController.setActiveOverlay(DisplayOverlay::None);
    });

    bleProvisioner.setPasskeyDisplayCallback([](uint32_t p_passkey) {
        Serial.printf("[BLE] Pairing passkey: %06lu\n", p_passkey);
        displayController.setProvisioningStatus(p_passkey);
    });

    bleProvisioner.setCredentialsCallback([](const WifiTypes::Ssid& p_ssid, const WifiTypes::Password& p_password) {
        if (!storage.saveWifiSSID(p_ssid)) {
            Serial.println("[BLE] Failed to save SSID");
            return;
        }
        if (!storage.saveWifiPass(p_password)) {
            Serial.println("[BLE] Failed to save password");
            return;
        }
        Serial.println("[BLE] New credentials saved");
        wifiManager.credentialsUpdated();
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

    mqttBridge.setOnCommandCallback([](std::string_view p_data) {
        commandHandler.enqueue(p_data);
    });

    commandHandler.start();
    claimButtonHandler.start();
    sensorConsumer.start();
    envSensor.start();
    wifiManager.start();
    healthReporter.start();

    vTaskDelete(nullptr);
}

void loop() {}
