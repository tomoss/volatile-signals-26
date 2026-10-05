#ifndef BINARY_SEMAPHORE_HPP
#define BINARY_SEMAPHORE_HPP

#include "vendor/arduino.hpp"
#include "vendor/freertos.hpp"

class BinarySemaphore {
public:
    BinarySemaphore() = default;
    ~BinarySemaphore() {
        if (m_handle) {
            vSemaphoreDelete(m_handle);
        }
    }
    BinarySemaphore(const BinarySemaphore&) = delete;
    BinarySemaphore& operator=(const BinarySemaphore&) = delete;
    BinarySemaphore(BinarySemaphore&&) = delete;
    BinarySemaphore& operator=(BinarySemaphore&&) = delete;

    [[nodiscard]] bool init() {
        m_handle = xSemaphoreCreateBinary();
        if (m_handle == nullptr) {
            Serial.println("Binary semaphore creation failed");
            return false;
        }
        return true;
    }

    void give() const { xSemaphoreGive(m_handle); }

    bool take(TickType_t p_ticksToWait = portMAX_DELAY) const { return xSemaphoreTake(m_handle, p_ticksToWait) == pdTRUE; }

    SemaphoreHandle_t handle() const { return m_handle; }

private:
    SemaphoreHandle_t m_handle = nullptr;
};

#endif // BINARY_SEMAPHORE_HPP
