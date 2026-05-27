#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <led.h>

constexpr const int16_t LED_PIN = 12;
constexpr const int16_t LED_ENABLE_PIN = 13;
constexpr const uint16_t LED_COUNT = 1;

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static LedState currentState = LedState::WIFI_DISCONNECTED;

void led_init() {
    pinMode(LED_ENABLE_PIN, OUTPUT);
    digitalWrite(LED_ENABLE_PIN, LOW);
    strip.begin();
    strip.setBrightness(20);
    led_set_state(LedState::WIFI_DISCONNECTED);
}

void led_set_state(LedState state) {
    currentState = state;
    switch (state) {
    case LedState::WIFI_DISCONNECTED:
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        strip.show();
        break;
    case LedState::WIFI_CONNECTED:
        strip.setPixelColor(0, strip.Color(255, 255, 0));
        strip.show();
        break;
    case LedState::MQTT_CONNECTED:
        strip.setPixelColor(0, strip.Color(0, 255, 0));
        strip.show();
        break;
    case LedState::MQTT_COMM:
        strip.setPixelColor(0, strip.Color(0, 0, 255));
        strip.show();
        break;
    }
}
