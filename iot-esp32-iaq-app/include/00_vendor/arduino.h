#ifndef VENDOR_ARDUINO_H
#define VENDOR_ARDUINO_H

/* Arduino.h pulls in FreeRTOS -> WString.h, which trip our strict
   -Wconversion/-Wsign-conversion/-Wattributes flags. Suppress at the source. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <Arduino.h>
#pragma GCC diagnostic pop

#endif // VENDOR_ARDUINO_H
