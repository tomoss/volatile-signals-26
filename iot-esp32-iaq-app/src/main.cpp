#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

#include "01_sensor/env_sensor.hpp"
#include "01_sensor/event_queue.hpp"
#include "01_sensor/sensor_data_processor.hpp"
#include "02_storage/storage.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "03_wifi/wifi_manager.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "05_ble/ble_provisioner.hpp"
#include "06_display/display_controller.hpp"
#include "07_claim/claim_handler.hpp"
#include "08_health/health_reporter.hpp"
#include "09_utils/device_info.hpp"
#include "09_utils/freertos_task.hpp"
#include "09_utils/mac_address.hpp"
#include "09_utils/ota_updater.hpp"
#include "09_utils/rtc.hpp"
#include "09_utils/time_sync.hpp"
#include "09_utils/wire_wrapper.hpp"

#include <esp_system.h>

// Delay duration to wait for board to stabilize
constexpr uint32_t DELAY_UNTIL_STABLE = 2000; // milliseconds

// Delay duration for reboot after failed init
constexpr uint32_t DELAY_UNTIL_RESTART = 6000; // milliseconds

constexpr std::size_t SENSOR_EVENT_QUEUE_LENGTH = 10;

/*****************************************************************/
/* Setup                                                         */
/*****************************************************************/
void setup() {
    Serial.begin(115200);
    delay(DELAY_UNTIL_STABLE); // Wait for board to stabilize
    Serial.println("Firmware version: " FIRMWARE_VERSION);

    static WireWrapper wireWrapper;
    static Storage storage;
    static EventQueue<SensorEvent, SENSOR_EVENT_QUEUE_LENGTH> sensorEventQueue;
    static EnvSensor envSensor(storage, sensorEventQueue);
    static WifiAdapter wifiAdapter(storage);
    static WifiManager wifiManager(wifiAdapter);
    static BleProvisioner bleProvisioner;
    static DisplayController displayController;
    static MqttBridge mqttBridge(storage, readMacAddress());
    static TimeSync timeSync;
    static RealTimeClock rtc;
    static ClaimHandler claimHandler(displayController, storage, mqttBridge);
    static HealthReporter healthReporter(mqttBridge, wifiAdapter);
    static FreeRtosTask sensorDataProcessorTask;
    static SensorDataProcessor sensorDataProcessor(sensorEventQueue, sensorDataProcessorTask);
    static OtaUpdater otaUpdater(envSensor, displayController);

    sensorDataProcessor.addConsumer(&mqttBridge);
    sensorDataProcessor.addConsumer(&displayController);

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
    const bool l_initOk = sensorEventQueue.init() && wireWrapper.init() && storage.init() && envSensor.init(wireWrapper) && wifiManager.init() &&
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
    sensorDataProcessor.start();
    envSensor.start();
    wifiManager.start();
    bleProvisioner.start();
    healthReporter.start();

    displayController.enableDisplay();
    wifiManager.enqueueWifiStart();

    vTaskDelete(nullptr);
}

void loop() {}
