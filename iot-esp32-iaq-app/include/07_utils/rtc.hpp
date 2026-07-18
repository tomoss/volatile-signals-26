#ifndef RTC_HPP
#define RTC_HPP

#include <ctime>
#include <optional>

#include "00_vendor/rtclib.hpp"

// PCF8563 RTC on the Seeed XIAO Expansion Base, battery-backed so it holds time across power loss/reset.
class RealTimeClock {
public:
    // p_wire must be `Wire`, already begun and clocked by main; it's injected (rather than
    // begun here) so main owns the shared bus's lifecycle.
    explicit RealTimeClock(TwoWire& p_wire) : m_wire(p_wire) {}
    ~RealTimeClock() = default;
    RealTimeClock(const RealTimeClock&) = delete;
    RealTimeClock& operator=(const RealTimeClock&) = delete;
    RealTimeClock(RealTimeClock&&) = delete;
    RealTimeClock& operator=(RealTimeClock&&) = delete;

    // Probes the PCF8563 on the I2C bus; returns false if it does not respond.
    [[nodiscard]] bool init();

    // Returns the RTC's held time as a UTC epoch, or nullopt if the RTC isn't present or its
    // voltage-low flag is set (battery never installed, dead, or was disconnected - the held
    // time can't be trusted).
    std::optional<time_t> read();

    // Writes p_epoch (UTC) to the RTC so it survives the next power loss. No-op if init()
    // didn't find the chip.
    void write(time_t p_epoch);

private:
    TwoWire& m_wire;
    RTC_PCF8563 m_rtc;
    bool m_present = false;
};

#endif // RTC_HPP
