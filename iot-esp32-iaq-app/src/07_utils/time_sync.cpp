#include "07_utils/time_sync.hpp"

constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr long GMT_OFFSET_SEC = 0; // keep the system clock in UTC
constexpr int DAYLIGHT_OFFSET_SEC = 0;

bool TimeSync::sync(uint32_t p_timeoutMs) {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

    struct tm l_timeinfo;
    if (!getLocalTime(&l_timeinfo, p_timeoutMs)) {
        Serial.println("[TimeSync] Failed to sync time via NTP");
        return false;
    }

    m_synced = true;

    char l_buf[32];
    strftime(l_buf, sizeof(l_buf), "%Y-%m-%d %H:%M:%S", &l_timeinfo);
    Serial.printf("[TimeSync] Time synced: %s UTC\n", l_buf);
    return true;
}
