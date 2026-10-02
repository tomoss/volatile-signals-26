#ifndef VENDOR_PREFERENCES_HPP
#define VENDOR_PREFERENCES_HPP

// Preferences.h pulls in Arduino.h -> FreeRTOS -> WString.h, which trip the strict
// -Wconversion/-Wsign-conversion/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <Preferences.h>
#pragma GCC diagnostic pop

#endif // VENDOR_PREFERENCES_HPP
