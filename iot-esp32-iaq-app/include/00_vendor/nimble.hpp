#ifndef VENDOR_NIMBLE_H
#define VENDOR_NIMBLE_H

// NimBLEDevice.h pulls in Arduino.h -> FreeRTOS -> WString.h, which trip our strict
// -Wconversion/-Wsign-conversion/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <NimBLEDevice.h>
#pragma GCC diagnostic pop

#endif // VENDOR_NIMBLE_H