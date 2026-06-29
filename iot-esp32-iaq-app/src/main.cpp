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

// Delay duration to wait for board to stabilize
constexpr uint32_t DELAY_UNTIL_STABLE = 2000; // milliseconds

// Delay duration for reboot after failed init
constexpr uint32_t DELAY_UNTIL_RESTART = 6000; // milliseconds

// I2C Fast-mode clock for the bus shared by the display and sensor. 400 kHz keeps each
// display refresh's bus-hold short so it barely perturbs sensor reads.
constexpr uint32_t I2C_BUS_CLOCK_HZ = 400000;

struct ConsumerTaskParams {
    EnvSensor* envSensor;
    DisplayController* displayController;
};

/*****************************************************************/
/* Tasks                                                         */
/*****************************************************************/
static void consumerTask(void* pvParameters) {
    auto* const l_params = static_cast<ConsumerTaskParams*>(pvParameters);
    auto* const l_envSensor = l_params->envSensor;
    auto* const l_displayController = l_params->displayController;

    for (;;) {
        SensorData l_data;
        if (xQueueReceive(l_envSensor->getQueue(), &l_data, pdMS_TO_TICKS(100))) {
            Serial.printf("[%lu] "
                          "IAQ=%.1f(acc:%d) "
                          "CO2eq=%.0fppm "
                          "VOCeq=%.2fppm "
                          "Gas=%.0fΩ "
                          "T=%.2fC "
                          "RH=%.2f%% "
                          "RawT=%.2fC "
                          "RawRH=%.2f%% "
                          "P=%.2fhPa\n",
                          millis(),
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

            if (l_displayController != nullptr && !std::isnan(l_data.iaq) && !std::isnan(l_data.temp)) {
                EnvDisplayState l_envState;
                l_envState.iaq = static_cast<uint16_t>(std::round(l_data.iaq));
                l_envState.temperatureC = static_cast<int8_t>(std::round(l_data.temp));
                l_envState.accuracy = static_cast<uint8_t>(l_data.iaqAccuracy);
                l_displayController->setEnvironment(l_envState);
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

    if (storage.init() == false) {
        Serial.println("Storage init failed, restarting...");
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

    envSensor.start();
    wifiManager.start();

    static ConsumerTaskParams consumerTaskParams{&envSensor, l_hasDisplay ? &displayController : nullptr};
    xTaskCreate(consumerTask, "consumer", 4096, &consumerTaskParams, 1, nullptr);

    vTaskDelete(nullptr);
}

void loop() {}
