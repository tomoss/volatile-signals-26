#pragma once

enum class LedState { WIFI_DISCONNECTED, WIFI_CONNECTED, MQTT_CONNECTED, MQTT_COMM };

void led_init();
void led_set_state(LedState state);
