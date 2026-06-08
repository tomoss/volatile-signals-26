#include <Arduino.h>
#include "app_wifi.h"
#include "led.h"

void setup() {
    Serial.begin(115200);
    led_init();
    wifi_init();
}

void loop() {
}