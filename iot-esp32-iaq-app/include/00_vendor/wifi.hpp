#ifndef VENDOR_WIFI_HPP
#define VENDOR_WIFI_HPP

// WiFi.h pulls in Network.h -> NetworkEvents.h, whose default arguments shadow its own
// member names and trip the strict -Wshadow flag.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <WiFi.h>
#pragma GCC diagnostic pop

#endif // VENDOR_WIFI_HPP
