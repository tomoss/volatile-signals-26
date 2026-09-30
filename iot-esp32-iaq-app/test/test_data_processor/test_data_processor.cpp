#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "01_sensor/data_processor.hpp"

struct FakeConsumer : Consumer {
    std::vector<SensorEvent> received;
    std::vector<int>* callOrder = nullptr;
    int id = 0;

    void update(SensorEvent p_event) override {
        received.push_back(p_event);
        if (callOrder) {
            callOrder->push_back(id);
        }
    }
};

class DataProcessorTest : public ::testing::Test {
protected:
    DataProcessor processor;
    FakeConsumer first;
    FakeConsumer second;
};

TEST_F(DataProcessorTest, NotifyReachesAllConsumers) {
    processor.addConsumer(&first);
    processor.addConsumer(&second);

    processor.notify(SensorEvent{SensorMode::LowPower});

    EXPECT_EQ(first.received.size(), 1U);
    EXPECT_EQ(second.received.size(), 1U);
}

TEST_F(DataProcessorTest, NotifyCallsConsumersInRegistrationOrder) {
    std::vector<int> l_order;
    first.callOrder = &l_order;
    first.id = 1;
    second.callOrder = &l_order;
    second.id = 2;
    processor.addConsumer(&second);
    processor.addConsumer(&first);

    processor.notify(SensorEvent{SensorMode::LowPower});

    EXPECT_EQ(l_order, (std::vector<int>{2, 1}));
}

TEST_F(DataProcessorTest, RemovedConsumerIsNotNotified) {
    processor.addConsumer(&first);
    processor.addConsumer(&second);
    processor.removeConsumer(&first);

    processor.notify(SensorEvent{SensorMode::LowPower});

    EXPECT_TRUE(first.received.empty());
    EXPECT_EQ(second.received.size(), 1U);
}

TEST_F(DataProcessorTest, NotifyWithoutConsumersIsNoop) {
    processor.notify(SensorEvent{SensorMode::LowPower});

    EXPECT_TRUE(first.received.empty());
}

TEST_F(DataProcessorTest, SensorDataIsForwardedUnchanged) {
    processor.addConsumer(&first);
    SensorData l_data;
    l_data.iaq = 42.5F;
    l_data.temp = 21.3F;
    l_data.iaqAccuracy = IAQAccuracy::High;
    l_data.timestamp = 1234;

    processor.notify(SensorEvent{l_data});

    ASSERT_EQ(first.received.size(), 1U);
    const auto* l_received = std::get_if<SensorData>(&first.received[0]);
    ASSERT_NE(l_received, nullptr);
    EXPECT_FLOAT_EQ(l_received->iaq, 42.5F);
    EXPECT_FLOAT_EQ(l_received->temp, 21.3F);
    EXPECT_EQ(l_received->iaqAccuracy, IAQAccuracy::High);
    EXPECT_EQ(l_received->timestamp, 1234);
}

TEST_F(DataProcessorTest, SensorModeIsForwardedUnchanged) {
    processor.addConsumer(&first);

    processor.notify(SensorEvent{SensorMode::Continuous});

    ASSERT_EQ(first.received.size(), 1U);
    const auto* l_received = std::get_if<SensorMode>(&first.received[0]);
    ASSERT_NE(l_received, nullptr);
    EXPECT_EQ(*l_received, SensorMode::Continuous);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
