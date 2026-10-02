#ifndef VENDOR_RTCLIB_HPP
#define VENDOR_RTCLIB_HPP

// RTClib.h and Wire.h trip the strict -Wconversion/-Wsign-conversion/-Wshadow flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <RTClib.h>
#include <Wire.h>
#pragma GCC diagnostic pop

#endif // VENDOR_RTCLIB_HPP
