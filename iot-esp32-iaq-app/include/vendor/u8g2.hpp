#ifndef VENDOR_U8G2_HPP
#define VENDOR_U8G2_HPP

// U8g2lib.h and Wire.h trip the strict -Wconversion/-Wsign-conversion/-Wshadow/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <U8g2lib.h>
#include <Wire.h>
#pragma GCC diagnostic pop

#endif // VENDOR_U8G2_HPP
