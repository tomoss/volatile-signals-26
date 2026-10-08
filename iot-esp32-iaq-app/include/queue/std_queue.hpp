#ifndef STD_QUEUE_HPP
#define STD_QUEUE_HPP

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

template<typename T, std::size_t N>
class StdQueue {
public:
    StdQueue() = default;
    ~StdQueue() = default;
    StdQueue(const StdQueue&) = delete;
    StdQueue& operator=(const StdQueue&) = delete;
    StdQueue(StdQueue&&) = delete;
    StdQueue& operator=(StdQueue&&) = delete;

    [[nodiscard]] bool init() { return true; }

    bool send(const T& p_item) {
        {
            const std::lock_guard<std::mutex> l_lock(m_mutex);
            if (m_items.size() >= N) {
                return false;
            }
            m_items.push(p_item);
        }
        m_cv.notify_one();
        return true;
    }

    bool receive(T& p_item) {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        if (!m_cv.wait_for(l_lock, RECEIVE_TIMEOUT, [this] { return !m_items.empty(); })) {
            return false;
        }
        p_item = m_items.front();
        m_items.pop();
        return true;
    }

private:
    static constexpr auto RECEIVE_TIMEOUT = std::chrono::milliseconds(100);

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_items;
};

#endif // STD_QUEUE_HPP
