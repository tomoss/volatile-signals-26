#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "00_vendor/arduino.hpp"
#include "00_vendor/freertos.hpp"

#include <cstddef>
#include <type_traits>

template<typename T, std::size_t N>
class Queue {
    // FreeRTOS copies items with memcpy.
    static_assert(std::is_trivially_copyable_v<T>, "Queue items must be trivially copyable");
    static_assert(N > 0, "Queue length must be at least 1");

public:
    Queue() = default;
    ~Queue() {
        if (m_handle != nullptr) {
            vQueueDelete(m_handle);
        }
    }
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    [[nodiscard]] bool init() {
        m_handle = xQueueCreate(static_cast<UBaseType_t>(N), static_cast<UBaseType_t>(sizeof(T)));
        if (m_handle == nullptr) {
            return false;
        }
        return true;
    }

    bool send(const T& p_item, TickType_t p_ticksToWait = 0) { return xQueueSend(m_handle, &p_item, p_ticksToWait) == pdTRUE; }

    // Replaces the single stored item, so it never blocks or fails. Only for length-1 queues.
    void overwrite(const T& p_item)
        requires(N == 1)
    {
        xQueueOverwrite(m_handle, &p_item);
    }

    // Returns false if nothing arrived within p_ticksToWait.
    bool receive(T& p_item, TickType_t p_ticksToWait = portMAX_DELAY) { return xQueueReceive(m_handle, &p_item, p_ticksToWait) == pdTRUE; }

private:
    QueueHandle_t m_handle = nullptr;
};

#endif // QUEUE_HPP
