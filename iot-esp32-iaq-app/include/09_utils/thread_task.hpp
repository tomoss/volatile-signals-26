#ifndef THREAD_TASK_HPP
#define THREAD_TASK_HPP

#include "09_utils/task.hpp"

#include <atomic>
#include <cstdint>
#include <thread>

class ThreadTask : public Task {
public:
    ThreadTask() = default;
    ~ThreadTask() override { stop(); }
    ThreadTask(const ThreadTask&) = delete;
    ThreadTask& operator=(const ThreadTask&) = delete;
    ThreadTask(ThreadTask&&) = delete;
    ThreadTask& operator=(ThreadTask&&) = delete;

    bool createAndStart(const char* /*p_name*/, TaskLoop p_taskLoop, uint32_t /*p_priority*/ = 1, uint32_t /*p_stackSize*/ = 4096) override {
        if (m_thread.joinable()) {
            return false;
        }
        m_running = true;
        m_thread = std::thread(std::move(p_taskLoop));
        return true;
    }

    bool running() const override { return m_running; }

    void stop() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};

#endif // THREAD_TASK_HPP
