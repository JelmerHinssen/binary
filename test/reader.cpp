#include <gtest/gtest.h>
#include "arrayreader.h"

using namespace std;
using namespace shilm::io;

class ReaderFixture : public testing::Test {
public:
    vector<uint8_t> data = {0xab, 0xcd, 0xef, 0x01};
    ArrayReader little{data, BinaryReader::LITTLE_ENDIAN, BinaryReader::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED};
    ArrayReader big{data, BinaryReader::BIG_ENDIAN, BinaryReader::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED};
};

TEST_F(ReaderFixture, readbyte) {
    EXPECT_EQ(little.readByte(), 0xab);
    EXPECT_EQ(little.readByte(), 0xcd);
    EXPECT_EQ(little.readByte(), 0xef);
    EXPECT_EQ(little.readByte(), 0x01);
}

TEST_F(ReaderFixture, readShortLittle) {
    EXPECT_EQ(little.readShort(), 0xcdab);
    EXPECT_EQ(little.readShort(), 0x01ef);
}

TEST_F(ReaderFixture, readShortBig) {
    EXPECT_EQ(big.readShort(), 0xabcd);
    EXPECT_EQ(big.readShort(), 0xef01);
}