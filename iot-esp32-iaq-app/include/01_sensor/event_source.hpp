#ifndef EVENT_SOURCE_HPP
#define EVENT_SOURCE_HPP

template<typename T>
class EventSource {
public:
    virtual ~EventSource() = default;

    virtual bool receive(T& p_item) = 0;
};

#endif // EVENT_SOURCE_HPP
