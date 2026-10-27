#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "../EOIRCommonLib/eoircommonlib.h"
#include "../IRSensorLib/irsensorlib.h"
#include "../EOSensorLib/eosensorlib.h"
using namespace testing;

TEST(EOIRTempTest, Test1)
{
    EXPECT_EQ(1, 1);

    ASSERT_THAT(0, Eq(0));
}

TEST(EOIRTempTest, Name_Test)
{
    EOIRCommonLib eoircommon;
    IRSensorLib   irSensor;
    EOSensorLib   eoSensor;

    EXPECT_STREQ(eoircommon.sensorName.c_str(), "");
    EXPECT_STREQ(irSensor.sensorName.c_str(), "IR Sensor");
    EXPECT_STREQ(eoSensor.sensorName.c_str(), "EO Sensor");

}
