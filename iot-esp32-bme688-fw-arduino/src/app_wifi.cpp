#include <Arduino.h>
#include <WiFi.h>
#include <config.h>
#include <led.h>

static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("WiFi connected");
        led_set_state(LedState::WIFI_CONNECTED);
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("WiFi disconnected, reason: %d\n", info.wifi_sta_disconnected.reason);
        led_set_state(LedState::WIFI_DISCONNECTED);
        break;
    default:
        break;
    }
}

void wifi_init() {
    WiFi.onEvent(onWifiEvent);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}