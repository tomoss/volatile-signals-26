#ifndef VENDOR_MQTT_CLIENT_H
#define VENDOR_MQTT_CLIENT_H

// esp-mqtt's mqtt_client.h pulls in esp_event.h -> FreeRTOS, which trip the strict
// -Wconversion/-Wsign-conversion/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <mqtt_client.h>
#pragma GCC diagnostic pop

#endif // VENDOR_MQTT_CLIENT_H
