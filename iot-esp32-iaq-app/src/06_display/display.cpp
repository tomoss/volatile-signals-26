#include "06_display/display.hpp"

// Address of the SSD1306 on the Seeed XIAO Expansion Base.
constexpr uint8_t I2C_ADDRESS = 0x3C;

// Used fonts for the two halves of the display.
constexpr const uint8_t* FIRST_HALF_FONT = u8g2_font_8x13_tf;
constexpr const uint8_t* SECOND_HALF_FONT = u8g2_font_logisoso18_tr;

// The SSD1306 is 128x64 pixels
constexpr uint8_t DISPLAY_WIDTH = 128;
constexpr uint8_t DISPLAY_HEIGHT = 64;

// Panel is split into top/bottom halves; these are the vertical center of each half.
constexpr uint8_t FIRST_HALF_CENTER_Y = DISPLAY_HEIGHT / 4;
constexpr uint8_t SECOND_HALF_CENTER_Y = DISPLAY_HEIGHT * 3 / 4;

static bool probeI2cAddress(TwoWire& p_wire, uint8_t p_address) {
    p_wire.beginTransmission(p_address);
    return p_wire.endTransmission() == 0;
}

bool Display::init() {
    // The bus is begun and clocked by main; here we only probe for the OLED.
    if (!probeI2cAddress(m_wire, I2C_ADDRESS)) {
        return false;
    }

    m_panel.setI2CAddress(static_cast<uint8_t>(I2C_ADDRESS << 1));
    if (!m_panel.begin()) {
        return false;
    }

    return true;
}

void Display::setMode(DisplayMode p_mode) {
    if (m_mode == p_mode) {
        return;
    }

    m_mode = p_mode;
    m_panel.setPowerSave(m_mode == DisplayMode::Off ? 1 : 0);
}

void Display::drawCentered(const char* p_text, const uint8_t* p_font, int p_y) {
    m_panel.setFont(p_font);
    m_panel.setFontPosCenter();
    const int l_width = static_cast<int>(m_panel.getUTF8Width(p_text));
    const int l_x = (DISPLAY_WIDTH - l_width) / 2;
    m_panel.drawUTF8(static_cast<u8g2_uint_t>(l_x > 0 ? l_x : 0), static_cast<u8g2_uint_t>(p_y), p_text);
}

void Display::renderHalves(const char* p_firstHalfText, const char* p_secondHalfText) {
    m_panel.clearBuffer();
    drawCentered(p_firstHalfText, FIRST_HALF_FONT, FIRST_HALF_CENTER_Y);
    drawCentered(p_secondHalfText, SECOND_HALF_FONT, SECOND_HALF_CENTER_Y);
    m_panel.sendBuffer();
}