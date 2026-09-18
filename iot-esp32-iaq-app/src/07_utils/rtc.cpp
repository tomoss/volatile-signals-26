#include "07_utils/rtc.hpp"

void RealTimeClock::init() {
    if (!m_rtc.begin(&m_wire.getRaw())) {
        Serial.println("RTC not found (continuing without RTC-backed boot time)");
        m_present = false;
        return;
    }
    m_present = true;
}

std::optional<time_t> RealTimeClock::read() {
    if (!m_present || m_rtc.lostPower()) {
        return std::nullopt;
    }
    return static_cast<time_t>(m_rtc.now().unixtime());
}

void RealTimeClock::write(time_t p_epoch) {
    if (!m_present || m_rtc.lostPower()) {
        return;
    }
    m_rtc.adjust(DateTime(static_cast<uint32_t>(p_epoch)));
}

void RealTimeClock::seedSystemClock() {
    if (const auto l_rtcTime = read()) {
        const struct timeval l_tv{*l_rtcTime, 0};
        settimeofday(&l_tv, nullptr);
        Serial.println("[RTC] Seeded system clock from RTC");
    }
}
