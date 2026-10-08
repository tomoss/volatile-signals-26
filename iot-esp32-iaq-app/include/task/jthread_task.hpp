#ifndef JTHREAD_TASK_HPP
#define JTHREAD_TASK_HPP

#include <cstdint>
#include <functional>
#include <stop_token>
#include <thread>

class JThreadTask {
public:
    using TaskLoop = std::function<void()>;

    JThreadTask() = default;
    ~JThreadTask() { stop(); }
    JThreadTask(const JThreadTask&) = delete;
    JThreadTask& operator=(const JThreadTask&) = delete;
    JThreadTask(JThreadTask&&) = delete;
    JThreadTask& operator=(JThreadTask&&) = delete;

    bool createAndStart(const char* /*p_name*/, TaskLoop p_taskLoop, uint32_t /*p_priority*/ = 1, uint32_t /*p_stackSize*/ = 4096) {
        if (m_thread.joinable()) {
            return false;
        }
        m_thread = std::jthread(std::move(p_taskLoop));
        return true;
    }

    bool running() const { return !m_stopSource.stop_requested(); }

    void stop() {
        m_stopSource.request_stop();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    std::stop_source m_stopSource;
    std::jthread m_thread;
};

#endif // JTHREAD_TASK_HPP
