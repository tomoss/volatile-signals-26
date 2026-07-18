#include "07_utils/rtc.hpp"

bool RealTimeClock::init() {
    // The bus is begun and clocked by main; begin() here only probes for the PCF8563.
    m_present = m_rtc.begin(&m_wire);
    return m_present;
}

std::optional<time_t> RealTimeClock::read() {
    if (!m_present || m_rtc.lostPower()) {
        return std::nullopt;
    }

    return static_cast<time_t>(m_rtc.now().unixtime());
}

void RealTimeClock::write(time_t p_epoch) {
    if (!m_present) {
        return;
    }

    m_rtc.adjust(DateTime(static_cast<uint32_t>(p_epoch)));
}
