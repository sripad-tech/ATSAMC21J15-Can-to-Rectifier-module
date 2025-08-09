#include <gtest/gtest.h>
#include "../src/app/rectifier.h"

TEST(Scaling, Fx10) {
    EXPECT_EQ(fx10_from_float(220.5f), 2205u);
    EXPECT_EQ(fx10_from_float( 20.0f),  200u);
    EXPECT_FLOAT_EQ(fx10_to_float(2210u), 221.0f);
    EXPECT_FLOAT_EQ(fx10_to_float( 210u),  21.0f);
}
