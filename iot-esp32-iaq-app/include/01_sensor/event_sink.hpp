#ifndef EVENT_SINK_HPP
#define EVENT_SINK_HPP

template<typename T>
class EventSink {
public:
    virtual ~EventSink() = default;

    virtual bool send(const T& p_item) = 0;
};

#endif // EVENT_SINK_HPP
