#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"
#include "00_vendor/http_update.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <variant>

#include "01_sensor/env_sensor.hpp"
#include "01_sensor/sensor_data.hpp"
#include "02_storage/storage.hpp"
#include "03_wifi/wifi_adapter.hpp"
#include "03_wifi/wifi_manager.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "04_mqtt/mqtt_types.hpp"
#include "05_ble/ble_provisioner.hpp"
#include "06_display/display_controller.hpp"
#include "07_utils/command.hpp"
#include "07_utils/device_health.hpp"
#include "07_utils/device_info.hpp"
#include "07_utils/rtc.hpp"
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

// Depth of the command queue fed by the MQTT event callback and drained by commandTask.
constexpr int COMMAND_QUEUE_SIZE = 8;

struct ConsumerTaskParams {
    EnvSensor* envSensor;
    DisplayController* displayController;
    MqttBridge* mqttBridge;
};

struct HealthTaskParams {
    MqttBridge* mqttBridge;
    WifiAdapter* wifiAdapter;
};

struct CommandTaskParams {
    EnvSensor* envSensor;
    MqttBridge* mqttBridge;
    Storage* storage;
    DisplayController* displayController;
};

// Fixed-size storage for the in-flight OTA URL, avoiding a heap allocation per request.
// s_otaInProgress guards it: only one OTA can be in flight at a time, so the buffer is
// never written by a new "ota" MQTT message while otaTask is still reading it.
static MqttTypes::Payload s_otaUrl{};
static std::atomic<bool> s_otaInProgress{false};

// The MQTT event callback (called on esp-mqtt's own task) only parses and enqueues; the
// actual command handling runs on commandTask so it never blocks the MQTT client task.
static QueueHandle_t s_commandQueue = nullptr;

/*****************************************************************/
/* Tasks                                                         */
/*****************************************************************/
static void otaTask(void* pvParameters) {
    const char* const l_url = static_cast<const char*>(pvParameters);

    WiFiClient l_client;
    Serial.printf("[OTA] Starting update from %s\n", l_url);

    const t_httpUpdate_return l_result = httpUpdate.update(l_client, l_url);

    switch (l_result) {
    case HTTP_UPDATE_OK:
        Serial.println("[OTA] Update OK, rebooting..."); // httpUpdate reboots automatically on success
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[OTA] No update available");
        break;
    case HTTP_UPDATE_FAILED:
        Serial.printf("[OTA] Failed: %s\n", httpUpdate.getLastErrorString().c_str());
        break;
    }

    s_otaInProgress.store(false);
    vTaskDelete(nullptr);
}

static void commandTask(void* pvParameters) {
    auto* const l_params = static_cast<CommandTaskParams*>(pvParameters);
    auto* const l_envSensor = l_params->envSensor;
    auto* const l_mqttBridge = l_params->mqttBridge;
    auto* const l_storage = l_params->storage;
    auto* const l_displayController = l_params->displayController;

    for (;;) {
        Command l_cmd;
        if (xQueueReceive(s_commandQueue, &l_cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        switch (l_cmd) {
        case Command::DeviceReboot:
            Serial.println("[CMD] Rebooting...");
            l_envSensor->requestModeChange(SensorMode::Disabled);
            l_mqttBridge->disconnect();
            esp_restart();
            break;
        case Command::SensorLowPower:
            Serial.println("[CMD] Switching sensor to Low Power mode");
            l_envSensor->requestModeChange(SensorMode::LowPower);
            break;
        case Command::SensorUltraLowPower:
            Serial.println("[CMD] Switching sensor to Ultra Low Power mode");
            l_envSensor->requestModeChange(SensorMode::UltraLowPower);
            break;
        case Command::DeviceClaimed:
            Serial.println("[CMD] Device claimed");
            l_storage->saveDeviceClaimStatus(true);
            if (l_displayController != nullptr) {
                l_displayController->setClaimedStatus(true);
            }
            break;
        case Command::DeviceUnclaimed:
            Serial.println("[CMD] Device unclaimed");
            l_storage->saveDeviceClaimStatus(false);
            if (l_displayController != nullptr) {
                l_displayController->setClaimedStatus(false);
            }
            break;
        case Command::Unknown:
            Serial.printf("[CMD] Unknown command received");
            break;
        }
    }
}

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
        l_health.timestamp = time(nullptr);

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
        SensorEvent l_event;
        if (!xQueueReceive(l_envSensor->getQueue(), &l_event, pdMS_TO_TICKS(100))) {
            continue;
        }

        if (const auto* l_mode = std::get_if<SensorMode>(&l_event)) {
            l_mqttBridge->sendSensorInfo(*l_mode);
            continue;
        }

        const auto& l_data = std::get<SensorData>(l_event);
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
                l_displayController->setEnvironment(static_cast<uint16_t>(std::round(l_data.iaq)),
                                                    static_cast<int8_t>(std::round(l_data.temp)),
                                                    static_cast<uint8_t>(l_data.iaqAccuracy));
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
    static RealTimeClock rtc(l_wire);

    static DeviceInfo deviceInfo;
    deviceInfo.firmwareVersion = FIRMWARE_VERSION;
    deviceInfo.chipModel = ESP.getChipModel();
    deviceInfo.chipRevision = ESP.getChipRevision();
    deviceInfo.chipCores = ESP.getChipCores();
    deviceInfo.resetReason = static_cast<uint8_t>(esp_reset_reason());
    deviceInfo.totalHeap = ESP.getHeapSize();

    if (storage.init() == false) {
        Serial.println("Storage init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    const bool l_hasRtc = rtc.init();
    if (!l_hasRtc) {
        Serial.println("RTC not found (continuing without RTC-backed boot time)");
    } else if (const auto l_rtcTime = rtc.read()) {
        const struct timeval l_tv{*l_rtcTime, 0};
        settimeofday(&l_tv, nullptr);
        Serial.println("[RTC] Seeded system clock from RTC");
    } else {
        Serial.println("[RTC] No valid time on RTC (battery low/never set)");
    }

    if (!envSensor.init(SensorMode::LowPower)) {
        Serial.println("EnvSensor init failed, restarting...");
        Serial.flush();
        delay(DELAY_UNTIL_RESTART);
        esp_restart();
    }

    // Generated once, ever
    if (!storage.loadClaimCode()) {
        ClaimCode l_code{};
        const uint32_t l_random = esp_random() % 1000000;
        snprintf(l_code.data(), l_code.size(), "%06lu", static_cast<unsigned long>(l_random));
        if (!storage.saveClaimCode(l_code)) {
            Serial.println("Failed to save claim code");
        }
    }

    // Created before wifiManager/mqttBridge can connect, since a command could otherwise
    // arrive (and be enqueued from the MQTT task) before this exists.
    s_commandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(Command));
    static CommandTaskParams commandTaskParams{&envSensor, &mqttBridge, &storage, &displayController};
    xTaskCreate(commandTask, "command", 4096, &commandTaskParams, 1, nullptr);

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

    wifiAdapter.setConnectedCallback([l_hasDisplay, l_hasRtc] {
        Serial.println("WiFi connected callback called");
        if (l_hasDisplay) {
            displayController.setWifiStatus(true);
        }
        if (timeSync.sync() && l_hasRtc) {
            rtc.write(time(nullptr));
        }
        mqttBridge.connect();
    });

    wifiAdapter.setDisconnectedCallback([l_hasDisplay] {
        Serial.println("WiFi disconnected callback called");
        if (l_hasDisplay) {
            displayController.setWifiStatus(false);
        }
    });

    wifiAdapter.setStartProvisioningCallback([l_hasDisplay] {
        bleProvisioner.start();
        if (l_hasDisplay) {
            displayController.setProvisioningStatus(0);
            displayController.setActiveOverlay(DisplayOverlay::Provisioning);
        }
    });

    wifiAdapter.setStopProvisioningCallback([l_hasDisplay] {
        bleProvisioner.stop();
        if (l_hasDisplay) {
            displayController.setActiveOverlay(DisplayOverlay::None);
        }
    });

    bleProvisioner.setPasskeyDisplayCallback([l_hasDisplay](uint32_t p_passkey) {
        Serial.printf("[BLE] Pairing passkey: %06lu\n", p_passkey);
        if (l_hasDisplay) {
            displayController.setProvisioningStatus(p_passkey);
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
            displayController.setMqttStatus(true);
        }
        mqttBridge.sendDeviceInfo(deviceInfo);
    });

    mqttBridge.setOnDisconnectedCallback([l_hasDisplay] {
        if (l_hasDisplay) {
            displayController.setMqttStatus(false);
        }
    });

    mqttBridge.setOnOtaCallback([l_hasDisplay](std::string_view p_url) {
        if (s_otaInProgress.exchange(true)) {
            Serial.println("[OTA] Update already in progress, ignoring");
            return;
        }

        Serial.println("[OTA] Preparing for update...");
        envSensor.requestModeChange(SensorMode::Disabled);
        if (l_hasDisplay) {
            displayController.disableDisplay();
        }

        const size_t l_len = std::min(p_url.size(), s_otaUrl.size() - 1);
        std::memcpy(s_otaUrl.data(), p_url.data(), l_len);
        s_otaUrl[l_len] = '\0';

        if (xTaskCreate(otaTask, "ota", 8192, s_otaUrl.data(), 1, nullptr) != pdPASS) {
            Serial.println("[OTA] Failed to create OTA task");
            s_otaInProgress.store(false);
            return;
        }
    });

    mqttBridge.setOnCommandCallback([](std::string_view p_data) {
        const Command l_cmd = parseCommand(p_data);
        if (xQueueSend(s_commandQueue, &l_cmd, 0) != pdTRUE) {
            Serial.println("[CMD] Command queue full, dropping command");
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
