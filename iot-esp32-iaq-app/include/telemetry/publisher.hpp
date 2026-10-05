#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include <list>

#include "telemetry/consumer.hpp"
#include "telemetry/telemetry_data.hpp"

class Publisher {
public:
    void addDataConsumer(TelemetryDataConsumer& p_consumer) { m_dataConsumers.push_back(&p_consumer); }
    void addInfoConsumer(TelemetryInfoConsumer& p_consumer) { m_infoConsumers.push_back(&p_consumer); }

protected:
    Publisher() = default;
    ~Publisher() = default;

    void notify(const TelemetryData& p_data) {
        for (auto* l_consumer : m_dataConsumers)
            l_consumer->update(p_data);
    }

    void notify(const TelemetryInfo& p_info) {
        for (auto* l_consumer : m_infoConsumers)
            l_consumer->update(p_info);
    }

private:
    std::list<TelemetryDataConsumer*> m_dataConsumers;
    std::list<TelemetryInfoConsumer*> m_infoConsumers;
};

#endif // PUBLISHER_HPP
