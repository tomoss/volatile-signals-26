#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "00_vendor/freertos.hpp"

// Thin RAII wrapper around a FreeRTOS mutex semaphore. Call init() once (e.g. from the
// owner's own init()) and bail out on failure; afterwards the handle is guaranteed valid,
// so MutexGuard can lock/unlock unconditionally.
class Mutex {
public:
    Mutex() = default;
    ~Mutex() {
        if (m_handle != nullptr) {
            vSemaphoreDelete(m_handle);
        }
    }
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    [[nodiscard]] bool init() {
        m_handle = xSemaphoreCreateMutex();
        return m_handle != nullptr;
    }

private:
    friend class MutexGuard;
    void take() const { xSemaphoreTake(m_handle, portMAX_DELAY); }
    void give() const { xSemaphoreGive(m_handle); }

    SemaphoreHandle_t m_handle = nullptr;
};

// RAII lock on a Mutex. Only construct one after Mutex::init() has succeeded.
class MutexGuard {
public:
    explicit MutexGuard(const Mutex& p_mutex) : m_mutex(p_mutex) { m_mutex.take(); }
    ~MutexGuard() { m_mutex.give(); }
    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;
    MutexGuard(MutexGuard&&) = delete;
    MutexGuard& operator=(MutexGuard&&) = delete;

private:
    const Mutex& m_mutex;
};

#endif // MUTEX_HPP
