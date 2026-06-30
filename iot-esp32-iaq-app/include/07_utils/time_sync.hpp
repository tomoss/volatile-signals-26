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

    // True once sync() has succeeded at least once. Before that, time(nullptr) silently
    // returns seconds-since-boot (no battery-backed RTC), not a real epoch - callers that
    // need a trustworthy timestamp should check this rather than assume.
    bool isSynced() const { return m_synced; }

private:
    bool m_synced = false;
};

#endif // TIME_SYNC_HPP
