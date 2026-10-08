#include "queue/std_queue.hpp"
#include "task/jthread_task.hpp"
#include "telemetry/consumer.hpp"
#include "telemetry/telemetry_publisher.hpp"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <thread>

constexpr std::size_t TELEMETRY_EVENT_QUEUE_LENGTH = 10;
constexpr int FAKE_READING_COUNT = 5;
constexpr auto FAKE_READING_PERIOD = std::chrono::seconds(1);

class ConsoleConsumer : public TelemetryDataConsumer, public TelemetryInfoConsumer {
public:
    void update(const TelemetryData& p_data) override {
        std::printf("[console] IAQ=%.1f(acc:%d) T=%.2fC RH=%.2f%% P=%.2fhPa\n",
                    p_data.iaq,
                    static_cast<int>(p_data.iaqAccuracy),
                    p_data.temp,
                    p_data.hum,
                    p_data.pressure);
    }

    void update(const TelemetryInfo& p_info) override { std::printf("[console] sensor mode=%d\n", static_cast<int>(p_info.sensorMode)); }
};

static TelemetryData makeFakeReading(int p_index) {
    TelemetryData l_data;
    l_data.iaq = 50.0F + static_cast<float>(p_index);
    l_data.co2 = 600.0F;
    l_data.voc = 0.5F;
    l_data.temp = 22.0F;
    l_data.hum = 45.0F;
    l_data.pressure = 1013.0F;
    l_data.iaqAccuracy = 3;
    l_data.timestamp = std::time(nullptr);
    return l_data;
}

int main() {
    ConsoleConsumer console;
    TelemetryPublisher<StdQueue<TelemetryEvent, TELEMETRY_EVENT_QUEUE_LENGTH>, JThreadTask> telemetryPublisher;

    if (!telemetryPublisher.init()) {
        return 1;
    }

    telemetryPublisher.addDataConsumer(console);
    telemetryPublisher.addInfoConsumer(console);
    telemetryPublisher.start();

    telemetryPublisher.enqueue(TelemetryInfo{2});
    for (int i = 0; i < FAKE_READING_COUNT; ++i) {
        telemetryPublisher.enqueue(makeFakeReading(i));
        std::this_thread::sleep_for(FAKE_READING_PERIOD);
    }

    return 0;
}
