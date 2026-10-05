#ifndef STD_TELEMETRY_QUEUE_HPP
#define STD_TELEMETRY_QUEUE_HPP

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

#include "telemetry/telemetry_data.hpp"
#include "telemetry/telemetry_sink.hpp"
#include "telemetry/telemetry_source.hpp"

template<std::size_t N>
class StdTelemetryQueue : public TelemetrySink, public TelemetrySource {
public:
    StdTelemetryQueue() = default;
    ~StdTelemetryQueue() override = default;
    StdTelemetryQueue(const StdTelemetryQueue&) = delete;
    StdTelemetryQueue& operator=(const StdTelemetryQueue&) = delete;
    StdTelemetryQueue(StdTelemetryQueue&&) = delete;
    StdTelemetryQueue& operator=(StdTelemetryQueue&&) = delete;

    void enqueue(const TelemetryEvent& p_event) override {
        {
            const std::lock_guard<std::mutex> l_lock(m_mutex);
            if (m_closed || m_events.size() >= N) {
                return;
            }
            m_events.push(p_event);
        }
        m_cv.notify_one();
    }

    bool receive(TelemetryEvent& p_event) override {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_cv.wait(l_lock, [this] { return m_closed || !m_events.empty(); });
        if (m_events.empty()) {
            return false;
        }
        p_event = m_events.front();
        m_events.pop();
        return true;
    }

    void close() {
        {
            const std::lock_guard<std::mutex> l_lock(m_mutex);
            m_closed = true;
        }
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<TelemetryEvent> m_events;
    bool m_closed = false;
};

#endif // STD_TELEMETRY_QUEUE_HPP
