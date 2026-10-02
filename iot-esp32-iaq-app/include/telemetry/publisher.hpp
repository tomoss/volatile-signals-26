#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include <list>

#include "telemetry/consumer.hpp"
#include "telemetry/telemetry_data.hpp"

class Publisher {
public:
    void addDataConsumer(TelemetryDataConsumer& observer) { m_dataObservers.push_back(&observer); }
    void addInfoConsumer(TelemetryInfoConsumer& observer) { m_modeObservers.push_back(&observer); }

protected:
    void notify(const TelemetryData& data) {
        for (auto* observer : m_dataObservers)
            observer->update(data);
    }

    void notify(const TelemetryInfo& info) {
        for (auto* observer : m_modeObservers)
            observer->update(info);
    }

private:
    std::list<TelemetryDataConsumer*> m_dataObservers;
    std::list<TelemetryInfoConsumer*> m_modeObservers;
};

#endif // PUBLISHER_HPP