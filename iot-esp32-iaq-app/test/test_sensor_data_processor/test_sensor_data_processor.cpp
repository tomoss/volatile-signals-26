#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <variant>
#include <vector>

#include "01_sensor/event_source.hpp"
#include "01_sensor/sensor_data_processor.hpp"
#include "09_utils/thread_task.hpp"

constexpr auto RECEIVE_TIMEOUT = std::chrono::milliseconds(10);
constexpr auto WAIT_TIMEOUT = std::chrono::seconds(2);

class FakeEventSource : public EventSource<SensorEvent> {
public:
    void push(const SensorEvent& p_event) {
        {
            const std::lock_guard<std::mutex> l_lock(m_mutex);
            m_events.push_back(p_event);
        }
        m_cv.notify_one();
    }

    bool receive(SensorEvent& p_item) override {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        if (!m_cv.wait_for(l_lock, RECEIVE_TIMEOUT, [this] { return !m_events.empty(); })) {
            return false;
        }
        p_item = m_events.front();
        m_events.pop_front();
        return true;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<SensorEvent> m_events;
};

class FakeConsumer : public Consumer {
public:
    void update(SensorEvent p_event) override {
        {
            const std::lock_guard<std::mutex> l_lock(m_mutex);
            m_received.push_back(p_event);
        }
        m_cv.notify_all();
    }

    std::vector<SensorEvent> waitFor(std::size_t p_count) {
        std::unique_lock<std::mutex> l_lock(m_mutex);
        m_cv.wait_for(l_lock, WAIT_TIMEOUT, [this, p_count] { return m_received.size() >= p_count; });
        return m_received;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<SensorEvent> m_received;
};

SensorData makeValidData() {
    SensorData l_data;
    l_data.iaq = 50.0F;
    l_data.co2 = 600.0F;
    l_data.voc = 0.5F;
    l_data.temp = 22.0F;
    l_data.hum = 45.0F;
    l_data.pressure = 1013.0F;
    l_data.gas = 100000.0F;
    l_data.rawTemp = 23.0F;
    l_data.rawHum = 44.0F;
    l_data.iaqAccuracy = IAQAccuracy::High;
    l_data.timestamp = 1000;
    return l_data;
}

class SensorDataProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor.addConsumer(&consumer);
        processor.start();
    }

    void TearDown() override { task.stop(); }

    void expectDropped(float SensorData::*p_field) {
        SensorData l_invalid = makeValidData();
        l_invalid.*p_field = NAN;
        SensorData l_marker = makeValidData();
        l_marker.timestamp = 2000;

        source.push(SensorEvent{l_invalid});
        source.push(SensorEvent{l_marker});

        const auto l_received = consumer.waitFor(1);
        ASSERT_EQ(l_received.size(), 1U);
        const auto* l_data = std::get_if<SensorData>(&l_received[0]);
        ASSERT_NE(l_data, nullptr);
        EXPECT_EQ(l_data->timestamp, 2000);
    }

    void expectForwarded(float SensorData::*p_field) {
        SensorData l_data = makeValidData();
        l_data.*p_field = NAN;

        source.push(SensorEvent{l_data});

        const auto l_received = consumer.waitFor(1);
        ASSERT_EQ(l_received.size(), 1U);
        EXPECT_TRUE(std::holds_alternative<SensorData>(l_received[0]));
    }

    ThreadTask task;
    FakeEventSource source;
    FakeConsumer consumer;
    SensorDataProcessor processor{source, task};
};

TEST_F(SensorDataProcessorTest, SensorModeIsForwarded) {
    source.push(SensorEvent{SensorMode::Continuous});

    const auto l_received = consumer.waitFor(1);

    ASSERT_EQ(l_received.size(), 1U);
    const auto* l_mode = std::get_if<SensorMode>(&l_received[0]);
    ASSERT_NE(l_mode, nullptr);
    EXPECT_EQ(*l_mode, SensorMode::Continuous);
}

TEST_F(SensorDataProcessorTest, ValidSensorDataIsForwarded) {
    source.push(SensorEvent{makeValidData()});

    const auto l_received = consumer.waitFor(1);

    ASSERT_EQ(l_received.size(), 1U);
    const auto* l_data = std::get_if<SensorData>(&l_received[0]);
    ASSERT_NE(l_data, nullptr);
    EXPECT_FLOAT_EQ(l_data->iaq, 50.0F);
    EXPECT_EQ(l_data->timestamp, 1000);
}

TEST_F(SensorDataProcessorTest, NanIaqIsDropped) {
    expectDropped(&SensorData::iaq);
}

TEST_F(SensorDataProcessorTest, NanTempIsDropped) {
    expectDropped(&SensorData::temp);
}

TEST_F(SensorDataProcessorTest, NanHumIsDropped) {
    expectDropped(&SensorData::hum);
}

TEST_F(SensorDataProcessorTest, NanPressureIsDropped) {
    expectDropped(&SensorData::pressure);
}

TEST_F(SensorDataProcessorTest, NanCo2IsDropped) {
    expectDropped(&SensorData::co2);
}

TEST_F(SensorDataProcessorTest, NanVocIsDropped) {
    expectDropped(&SensorData::voc);
}

TEST_F(SensorDataProcessorTest, NanGasIsForwarded) {
    expectForwarded(&SensorData::gas);
}

TEST_F(SensorDataProcessorTest, NanRawTempIsForwarded) {
    expectForwarded(&SensorData::rawTemp);
}

TEST_F(SensorDataProcessorTest, NanRawHumIsForwarded) {
    expectForwarded(&SensorData::rawHum);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
