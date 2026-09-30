#ifndef DATA_PROCESSOR_HPP
#define DATA_PROCESSOR_HPP

#include <list>

#include "01_sensor/consumer.hpp"

class DataProcessor {
public:
    void addConsumer(Consumer* p_consumer) { m_consumers.push_back(p_consumer); }

    void removeConsumer(Consumer* p_consumer) { m_consumers.remove(p_consumer); }

    void notify(SensorEvent p_event) {
        for (Consumer* consumer : m_consumers) {
            consumer->update(p_event);
        }
    }

private:
    std::list<Consumer*> m_consumers;
};
#endif // DATA_PROCESSOR_HPP