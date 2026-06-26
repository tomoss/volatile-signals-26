#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "00_vendor/u8g2.hpp"

// On: panel stays powered and renders frames.
// Off: panel sleeps and rendering calls become no-ops.
enum class DisplayMode : uint8_t { Off = 0, On = 1 };

// SSD1306 128x64 OLED on the Seeed XIAO Expansion Base (I2C, address 0x3C).
class Display {
public:
    // U8g2's HW I2C is bound to the global `Wire`, so p_wire must be `Wire`; it's injected
    // (rather than begun here) so main owns the shared bus's lifecycle and clock.
    explicit Display(TwoWire& p_wire) : m_wire(p_wire) {}
    ~Display() = default;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&) = delete;
    Display& operator=(Display&&) = delete;

    // Probes the OLED on the I2C bus; returns false if it does not respond.
    [[nodiscard]] bool init();

    // Off puts the SSD1306 controller to sleep: it drops to ~10uA and the panel/charge-pump
    // are off, but GDDRAM (and so the buffer contents) is retained for the next wake.
    void setMode(DisplayMode p_mode);

    // Clears the buffer, centers p_firstHalfText/p_secondHalfText in their own half of the panel and transmits. No-op while off.
    void renderHalves(const char* p_firstHalfText, const char* p_secondHalfText);

private:
    // Draws p_text centered horizontally at p_y using p_font.
    void drawCentered(const char* p_text, const uint8_t* p_font, int p_y);

    DisplayMode m_mode{DisplayMode::Off};
    TwoWire& m_wire;

    // U8G2_RO - no rotation, origin at top-left, x right, y down.
    // U8X8_PIN_NONE - no reset pin, the SSD1306 has a built-in power-on reset.
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C m_panel{U8G2_R0, U8X8_PIN_NONE};
};

#endif // DISPLAY_HPP