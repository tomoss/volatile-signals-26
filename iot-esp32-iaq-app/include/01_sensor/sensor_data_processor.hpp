#ifndef SENSOR_DATA_PROCESSOR_HPP
#define SENSOR_DATA_PROCESSOR_HPP

#include "01_sensor/data_processor.hpp"
#include "01_sensor/event_source.hpp"
#include "01_sensor/sensor_types.hpp"
#include "09_utils/task.hpp"

class SensorDataProcessor : public DataProcessor {
public:
    SensorDataProcessor(EventSource<SensorEvent>& p_source, Task& p_task)
        : m_source(p_source)
        , m_task(p_task) {}
    ~SensorDataProcessor() = default;
    SensorDataProcessor(const SensorDataProcessor&) = delete;
    SensorDataProcessor& operator=(const SensorDataProcessor&) = delete;
    SensorDataProcessor(SensorDataProcessor&&) = delete;
    SensorDataProcessor& operator=(SensorDataProcessor&&) = delete;

    void start();

private:
    void loop();
    void handle(const SensorEvent& p_event);

    EventSource<SensorEvent>& m_source;
    Task& m_task;
};

#endif // SENSOR_DATA_PROCESSOR_HPP
