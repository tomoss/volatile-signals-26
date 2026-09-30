#ifndef EVENT_QUEUE_HPP
#define EVENT_QUEUE_HPP

#include "01_sensor/event_sink.hpp"
#include "01_sensor/event_source.hpp"
#include "09_utils/queue.hpp"

#include <cstddef>

template<typename T, std::size_t N>
class EventQueue : public EventSink<T>, public EventSource<T> {
public:
    [[nodiscard]] bool init() { return m_queue.init(); }

    bool send(const T& p_item) override { return m_queue.send(p_item); }

    bool receive(T& p_item) override { return m_queue.receive(p_item); }

private:
    Queue<T, N> m_queue;
};

#endif // EVENT_QUEUE_HPP
