#ifndef VENDOR_HTTP_UPDATE_HPP
#define VENDOR_HTTP_UPDATE_HPP

// NetworkEvents.h has constructor parameter names that shadow member variables,
// tripping -Wshadow.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#pragma GCC diagnostic pop

#endif // VENDOR_HTTP_UPDATE_HPP