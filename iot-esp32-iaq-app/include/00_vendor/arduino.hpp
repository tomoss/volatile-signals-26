#ifndef VENDOR_ARDUINO_HPP
#define VENDOR_ARDUINO_HPP

// Arduino.h pulls in FreeRTOS -> WString.h, which trip the strict
// -Wconversion/-Wsign-conversion/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <Arduino.h>
#pragma GCC diagnostic pop

#endif // VENDOR_ARDUINO_HPP