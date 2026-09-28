#ifndef TIME_SYNC_HPP
#define TIME_SYNC_HPP

#include "00_vendor/arduino.hpp"

inline constexpr uint32_t TIME_SYNC_DEFAULT_TIMEOUT_MS = 10000; // 10 seconds

// Wraps the ESP32 SNTP client. WiFi alone doesn't carry time - sync() kicks off NTP and
// blocks until the system clock is set (or timeout), so we need to call it from a task context once
// an IP connection is up, never from a WiFi event callback.
class TimeSync {
public:
    bool sync(uint32_t p_timeoutMs = TIME_SYNC_DEFAULT_TIMEOUT_MS);

    // True once sync() has succeeded at least once this boot, i.e. time(nullptr) is
    // confirmed accurate via NTP. Before that, the system clock may already hold a
    // plausible epoch seeded from the battery-backed RTC (see Rtc), but that value can be
    // stale if the device was powered off for a while - callers that need network-confirmed
    // accuracy should check this rather than assume.
    bool isSynced() const { return m_synced; }

private:
    bool m_synced = false;
};

#endif // TIME_SYNC_HPP
