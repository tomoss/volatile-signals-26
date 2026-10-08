#ifndef FREERTOS_TASK_HPP
#define FREERTOS_TASK_HPP

#include "vendor/arduino.hpp"
#include "vendor/freertos.hpp"

#include <cstdint>
#include <functional>

class FreeRtosTask {
public:
    using TaskLoop = std::function<void()>;

    FreeRtosTask() = default;
    ~FreeRtosTask() {
        if (m_handle) {
            vTaskDelete(m_handle);
            m_handle = nullptr;
        }
    }
    FreeRtosTask(const FreeRtosTask&) = delete;
    FreeRtosTask& operator=(const FreeRtosTask&) = delete;
    FreeRtosTask(FreeRtosTask&&) = delete;
    FreeRtosTask& operator=(FreeRtosTask&&) = delete;

    bool createAndStart(const char* p_name, TaskLoop p_taskLoop, uint32_t p_priority = 1, uint32_t p_stackSize = 4096) {
        m_taskLoop = std::move(p_taskLoop);
        if (pdPASS != xTaskCreate(taskEntry, p_name, p_stackSize, this, static_cast<UBaseType_t>(p_priority), &m_handle)) {
            Serial.printf("%s task creation failed\n", p_name);
            return false;
        }
        return true;
    }

    bool running() const { return true; }

    TaskHandle_t handle() const { return m_handle; }

private:
    static void taskEntry(void* p_parameter) { static_cast<FreeRtosTask*>(p_parameter)->m_taskLoop(); }

    TaskLoop m_taskLoop;
    TaskHandle_t m_handle = nullptr;
};

#endif // FREERTOS_TASK_HPP
