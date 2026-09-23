#ifndef TASK_HPP
#define TASK_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

#include <functional>

class Task {
public:
    using Loop = std::function<void()>;

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

    bool start(const char* p_name, uint32_t p_stackSize, UBaseType_t p_priority, Loop p_loop) {
        m_loop = std::move(p_loop);
        if (pdPASS != xTaskCreate(taskEntry, p_name, p_stackSize, this, p_priority, &m_handle)) {
            Serial.printf("%s task creation failed\n", p_name);
            return false;
        }
        return true;
    }

    // For callers that need the raw handle - e.g. to notify it from an ISR.
    TaskHandle_t handle() const { return m_handle; }

private:
    static void taskEntry(void* p_parameter) { static_cast<Task*>(p_parameter)->m_loop(); }

    Loop m_loop;
    TaskHandle_t m_handle = nullptr;
};

#endif // TASK_HPP
