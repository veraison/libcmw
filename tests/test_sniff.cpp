#include <gtest/gtest.h>

#include "cmw/cmw.hpp"

using cmw::Format;
using cmw::sniff;

TEST(SniffTest, DetectFormats) {
    EXPECT_EQ(sniff(std::vector<uint8_t>{'[', '3', '0'}), Format::JSONRecord);
    EXPECT_EQ(sniff(std::vector<uint8_t>{'{', '"'}), Format::JSONCollection);
    EXPECT_EQ(sniff(std::vector<uint8_t>{0x82, 0x19, 0x75, 0x31}), Format::CBORRecord);
    EXPECT_EQ(sniff(std::vector<uint8_t>{0xa1, 0x01}), Format::CBORCollection);
    EXPECT_EQ(sniff(std::vector<uint8_t>{0xda, 0x63, 0x74, 0x76, 0x32}), Format::CBORTag);
}
