#include <gtest/gtest.h>
#include "../src/app/rectifier.h"

TEST(EID, TxVector) {
    uint32_t eid = rect_pack_eid(1,1,0x9D,1,1,0x08);
    EXPECT_EQ(eid, 0x0119D308u);
}
TEST(EID, AckVector) {
    uint32_t eid = rect_pack_eid(2,0,0xA2,0,0,0x08);
    EXPECT_EQ(eid, 0x020A2008u);
}
TEST(EID, FilterMask) {
    uint32_t match = rect_pack_eid(2,0,0,0,0,0);
    uint32_t mask  = ((uint32_t)0x1F << 24) | ((uint32_t)0x01 << 8);
    EXPECT_EQ(match, 0x02000000u);
    EXPECT_EQ(mask,  0x1F000100u);
}
