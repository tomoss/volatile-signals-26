#ifndef WIRE_WRAPPER_HPP
#define WIRE_WRAPPER_HPP

#include "00_vendor/arduino.hpp"

// I2C Fast-mode clock for the bus shared by the display and sensor. 400 kHz keeps each
// display refresh's bus-hold short so it barely perturbs sensor reads.
constexpr uint32_t I2C_BUS_CLOCK_HZ = 400000;

class WireWrapper {
public:
    bool init() {
        if (m_wire.begin() && m_wire.setClock(I2C_BUS_CLOCK_HZ)) {
            return true;
        }

        Serial.println("I2C BUS init failed");
        return false;
    }

    TwoWire& getRaw() { return m_wire; }

private:
    TwoWire& m_wire = Wire;
};

#endif // WIRE_WRAPPER_HPP