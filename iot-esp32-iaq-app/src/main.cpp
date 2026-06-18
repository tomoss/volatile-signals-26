#include "00_vendor/arduino.h"
#include "00_vendor/freertos.h"

#include "01_sensor/env_sensor.h"
#include "01_sensor/sensor_data.h"
#include "02_storage/storage.h"

/*****************************************************************/
/* Tasks                                                         */
/*****************************************************************/
static void consumerTask(void* pvParameters) {
    auto* const l_envSensor = static_cast<EnvSensor*>(pvParameters);

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
        }
    }
}

/*****************************************************************/
/* Setup                                                         */
/*****************************************************************/
void setup() {
    Serial.begin(115200);
    delay(3000); // Wait for board to stabilize

    static Storage storage;

    static EnvSensor envSensor(storage);

    if (!envSensor.init(SensorMode::LowPower)) {
        Serial.println("EnvSensor init failed, restarting...");
        Serial.flush();
        delay(1000);
        esp_restart();
    }

    envSensor.begin();

    xTaskCreate(consumerTask, "consumer", 4096, &envSensor, 1, nullptr);

    vTaskDelete(nullptr);
}

void loop() {}
