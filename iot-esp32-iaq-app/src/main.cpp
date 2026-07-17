#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

#include "01_sensor/env_sensor.hpp"
#include "01_sensor/sensor_data.hpp"
#include "02_storage/storage.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "03_wifi/wifi_manager.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "05_ble/ble_provisioner.hpp"
#include "06_display/display_controller.hpp"
#include "07_utils/command.hpp"
#include "07_utils/device_health.hpp"
#include "07_utils/device_info.hpp"
#include "07_utils/time_sync.hpp"

#include <esp_system.h>

// Delay duration to wait for board to stabilize
constexpr uint32_t DELAY_UNTIL_STABLE = 2000; // milliseconds

// Delay duration for reboot after failed init
constexpr uint32_t DELAY_UNTIL_RESTART = 6000; // milliseconds

// I2C Fast-mode clock for the bus shared by the display and sensor. 400 kHz keeps each
// display refresh's bus-hold short so it barely perturbs sensor reads.
constexpr uint32_t I2C_BUS_CLOCK_HZ = 400000;

// How often to publish device health (RSSI/heap/uptime) - diagnostic data
constexpr uint32_t HEALTH_PUBLISH_INTERVAL_MS = 60000; // 60 seconds

struct ConsumerTaskParams {
    EnvSensor* envSensor;
    DisplayController* displayController;
    MqttBridge* mqttBridge;
};

struct HealthTaskParams {
    MqttBridge* mqttBridge;
    WifiAdapter* wifiAdapter;
};

/*****************************************************************/
/* Tasks                                                         */
/*****************************************************************/
static void healthTask(void* pvParameters) {
    auto* const l_params = static_cast<HealthTaskParams*>(pvParameters);
    auto* const l_mqttBridge = l_params->mqttBridge;
    auto* const l_wifiAdapter = l_params->wifiAdapter;

    for (;;) {
        DeviceHealth l_health;
        l_health.rssi = l_wifiAdapter->getRSSI();
        l_health.heap = ESP.getFreeHeap();
        l_health.minHeap = ESP.getMinFreeHeap();
        l_health.uptime = millis() / 1000;

        l_mqttBridge->sendDeviceHealth(l_health);
        vTaskDelay(pdMS_TO_TICKS(HEALTH_PUBLISH_INTERVAL_MS));
    }
}

static void consumerTask(void* pvParameters) {
    auto* const l_params = static_cast<ConsumerTaskParams*>(pvParameters);
    auto* const l_envSensor = l_params->envSensor;
    auto* const l_displayController = l_params->displayController;
    auto* const l_mqttBridge = l_params->mqttBridge;

    for (;;) {
        SensorData l_data;
        if (xQueueReceive(l_envSensor->getQueue(), &l_data, pdMS_TO_TICKS(100))) {
            Serial.printf("[%lld] "
                          "IAQ=%.1f(acc:%d) "
                          "CO2eq=%.0fppm "
                          "VOCeq=%.2fppm "
                          "Gas=%.0fΩ "
                          "T=%.2fC "
                          "RH=%.2f%% "
                          "RawT=%.2fC "
                          "RawRH=%.2f%% "
                          "P=%.2fhPa\n",
                          l_data.timestamp,
                          l_data.iaq,
                          static_cast<int>(l_data.iaqAccuracy),
                          l_data.co2,
                          l_data.voc,
                          l_data.gas,
                          l_data.temp,
                          l_data.hum,
                          l_data.rawTemp,
                          l_data.rawHum,
                          l_data.pressure);

            if (!std::isnan(l_data.iaq) && !std::isnan(l_data.temp) && !std::isnan(l_data.hum) && !std::isnan(l_data.pressure) &&
                !std::isnan(l_data.co2) && !std::isnan(l_data.voc)) {
                l_mqttBridge->sendSensorData(l_data);

                if (l_displayController != nullptr) {
                    EnvDisplayState l_envState;
                    l_envState.iaq = static_cast<uint16_t>(std::round(l_data.iaq));
                    l_envState.temperatureC = static_cast<int8_t>(std::round(l_data.temp));
                    l_envState.accuracy = static_cast<uint8_t>(l_data.iaqAccuracy);
                    l_displayController->setEnvironment(l_envState);
                }
            }
        }
    }
}

/*****************************************************************/
/* Setup                                                         */
/*****************************************************************/
void setup() {
    Serial.begin(115200);
    delay(DELAY_UNTIL_STABLE); // Wait for board to stabilize
    Serial.println("Firmware version: " FIRMWARE_VERSION);

    // Own the shared I2C bus here, then inject it into every device on it (display + sensor)
    // so they share one consistently-clocked bus instead of each calling Wire.begin().
    TwoWire& l_wire = Wire;

    if (!l_wire.begin()) {
        Serial.println("I2C bus init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    l_wire.setClock(I2C_BUS_CLOCK_HZ);

    static Storage storage;
    static EnvSensor envSensor(storage, l_wire);
    static WifiAdapter wifiAdapter(storage);
    static WifiManager wifiManager(wifiAdapter);
    static BleProvisioner bleProvisioner;
    static DisplayController displayController(l_wire);
    static MqttBridge mqttBridge(storage);
    static TimeSync timeSync;

    static DeviceInfo deviceInfo;
    deviceInfo.firmwareVersion = FIRMWARE_VERSION;
    deviceInfo.chipModel = ESP.getChipModel();
    deviceInfo.chipRevision = ESP.getChipRevision();
    deviceInfo.chipCores = ESP.getChipCores();
    deviceInfo.resetReason = static_cast<uint8_t>(esp_reset_reason());

    if (storage.init() == false) {
        Serial.println("Storage init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    if (!envSensor.init(SensorMode::LowPower)) {
        Serial.println("EnvSensor init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    if (!wifiManager.init()) {
        Serial.println("WiFiManager init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    if (!bleProvisioner.init()) {
        Serial.println("BleProvisioner init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    if (!mqttBridge.init(true)) {
        Serial.println("MqttBridge init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    const bool l_hasDisplay = displayController.init();
    if (!l_hasDisplay) {
        Serial.println("Display init failed (continuing without display)");
    } else {
        displayController.enableDisplay();
    }

    wifiAdapter.setConnectedCallback([l_hasDisplay] {
        Serial.println("WiFi connected callback called");
        if (l_hasDisplay) {
            displayController.setWifiStatus(WifiDisplayState{true});
        }
        timeSync.sync();
        mqttBridge.connect();
    });

    wifiAdapter.setDisconnectedCallback([l_hasDisplay] {
        Serial.println("WiFi disconnected callback called");
        if (l_hasDisplay) {
            displayController.setWifiStatus(WifiDisplayState{false});
        }
    });

    wifiAdapter.setStartProvisioningCallback([l_hasDisplay] {
        bleProvisioner.start();
        if (l_hasDisplay) {
            displayController.setProvisioningStatus(ProvisionDisplayState{true, 0});
        }
    });

    wifiAdapter.setStopProvisioningCallback([l_hasDisplay] {
        bleProvisioner.stop();
        if (l_hasDisplay) {
            displayController.setProvisioningStatus(ProvisionDisplayState{false, 0});
        }
    });

    bleProvisioner.setPasskeyDisplayCallback([l_hasDisplay](uint32_t p_passkey) {
        Serial.printf("[BLE] Pairing passkey: %06lu\n", p_passkey);
        if (l_hasDisplay) {
            displayController.setProvisioningStatus(ProvisionDisplayState{true, p_passkey});
        }
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

    mqttBridge.setOnConnectedCallback([l_hasDisplay] {
        if (l_hasDisplay) {
            displayController.setMqttStatus(MqttDisplayState{true});
        }
        mqttBridge.sendDeviceInfo(deviceInfo);
    });

    mqttBridge.setOnDisconnectedCallback([l_hasDisplay] {
        if (l_hasDisplay) {
            displayController.setMqttStatus(MqttDisplayState{false});
        }
    });

    mqttBridge.setOnCommandCallback([](std::string_view p_data) {
        switch (parseCommand(p_data)) {
        case Command::Reboot:
            Serial.println("[CMD] Rebooting...");
            esp_restart();
            return;
        case Command::SensorLowPower:
            Serial.println("[CMD] Switching sensor to Low Power mode");
            envSensor.requestModeChange(SensorMode::LowPower);
            return;
        case Command::SensorUltraLowPower:
            Serial.println("[CMD] Switching sensor to Ultra Low Power mode");
            envSensor.requestModeChange(SensorMode::UltraLowPower);
            return;
        case Command::Unknown:
            Serial.printf("[CMD] Unknown command received");
            return;
        }
    });

    envSensor.start();
    wifiManager.start();

    static ConsumerTaskParams consumerTaskParams{&envSensor, l_hasDisplay ? &displayController : nullptr, &mqttBridge};
    xTaskCreate(consumerTask, "consumer", 4096, &consumerTaskParams, 1, nullptr);

    static HealthTaskParams healthTaskParams{&mqttBridge, &wifiAdapter};
    xTaskCreate(healthTask, "health", 4096, &healthTaskParams, 1, nullptr);

    vTaskDelete(nullptr);
}

void loop() {}
