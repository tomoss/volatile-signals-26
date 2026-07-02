#ifndef VENDOR_MQTT_CLIENT_HPP
#define VENDOR_MQTT_CLIENT_HPP

// mqtt_client.h pulls in esp_event.h -> FreeRTOS -> esp_cpu.h, which on RISC-V targets
// (e.g. esp32c6) has a CSR-read macro that narrows unsigned long -> int, tripping the
// strict -Wsign-conversion flag. Xtensa targets (e.g. esp32s3) don't hit this path.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <mqtt_client.h>
#pragma GCC diagnostic pop

#endif // VENDOR_MQTT_CLIENT_HPP
