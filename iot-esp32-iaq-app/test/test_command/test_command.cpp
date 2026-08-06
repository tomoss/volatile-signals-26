#include <gtest/gtest.h>

#include "07_utils/command.hpp"

TEST(ParseCommandTest, RebootParses) {
    EXPECT_EQ(Command::DeviceReboot, parseCommand("device_reboot"));
}

TEST(ParseCommandTest, SensorLowPowerParses) {
    EXPECT_EQ(Command::SensorLowPower, parseCommand("sensor_lp"));
}

TEST(ParseCommandTest, SensorUltraLowPowerParses) {
    EXPECT_EQ(Command::SensorUltraLowPower, parseCommand("sensor_ulp"));
}

TEST(ParseCommandTest, ClaimedParses) {
    EXPECT_EQ(Command::DeviceClaimed, parseCommand("device_claimed"));
}

TEST(ParseCommandTest, UnclaimedParses) {
    EXPECT_EQ(Command::DeviceUnclaimed, parseCommand("device_unclaimed"));
}

TEST(ParseCommandTest, UnrecognizedStringIsUnknown) {
    EXPECT_EQ(Command::Unknown, parseCommand("not_a_command"));
}

TEST(ParseCommandTest, EmptyStringIsUnknown) {
    EXPECT_EQ(Command::Unknown, parseCommand(""));
}

TEST(ParseCommandTest, IsCaseSensitive) {
    EXPECT_EQ(Command::Unknown, parseCommand("Reboot"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}