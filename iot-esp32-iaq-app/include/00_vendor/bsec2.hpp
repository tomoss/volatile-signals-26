#ifndef VENDOR_BSEC2_H
#define VENDOR_BSEC2_H

// bsec2.h pulls in Arduino.h -> FreeRTOS -> WString.h, which trip the strict
// -Wconversion/-Wsign-conversion/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <bsec2.h>
#pragma GCC diagnostic pop

#endif // VENDOR_BSEC2_H