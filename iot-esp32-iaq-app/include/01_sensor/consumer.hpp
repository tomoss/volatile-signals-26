#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include "01_sensor/sensor_types.hpp"

class Consumer {
public:
    virtual ~Consumer() = default;

    virtual void update(SensorEvent p_event) = 0;
};

#endif // CONSUMER_HPP