#ifndef TASK_HPP
#define TASK_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

#include <functional>

class Task {
public:
    using TaskLoop = std::function<void()>;

    Task() = default;
    ~Task() {
        if (m_handle != nullptr) {
            vTaskDelete(m_handle);
            m_handle = nullptr;
        }
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) = delete;
    Task& operator=(Task&&) = delete;

    bool createAndStart(const char* p_name, TaskLoop p_taskLoop, UBaseType_t p_priority = 1, uint32_t p_stackSize = 4096) {
        m_taskLoop = std::move(p_taskLoop);
        if (pdPASS != xTaskCreate(taskEntry, p_name, p_stackSize, this, p_priority, &m_handle)) {
            Serial.printf("%s task creation failed\n", p_name);
            return false;
        }
        return true;
    }

    TaskHandle_t handle() const { return m_handle; }

private:
    static void taskEntry(void* p_parameter) { static_cast<Task*>(p_parameter)->m_taskLoop(); }

    TaskLoop m_taskLoop;
    TaskHandle_t m_handle = nullptr;
};

#endif // TASK_HPP
