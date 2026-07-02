#pragma once

// Minimal native-only stand-in for Arduino's global Serial object. Only included when
// compiling for env:native (see platformio.ini); the real Arduino.h is used for esp32s3.
// Just enough surface for wifi_sm.hpp's logging calls - no actual hardware exists to log to.
struct SerialStub {
    void println(const char*) const {}

    template<typename... Args>
    void printf(const char*, Args&&...) const {}
};

inline SerialStub Serial;
