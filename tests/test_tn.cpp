#include <gtest/gtest.h>

#include "cmw/tn.hpp"

using cmw::content_format_from_tn;
using cmw::kCfMax;
using cmw::kCfMin;
using cmw::kTnMax;
using cmw::kTnMin;
using cmw::tn_from_content_format;

TEST(TnTest, RoundTrip) {
    for (uint16_t cf = kCfMin; cf < kCfMax; ++cf) {
        auto tn = tn_from_content_format(cf);
        ASSERT_TRUE(tn);
        auto actual = content_format_from_tn(tn.value());
        ASSERT_TRUE(actual);
        EXPECT_EQ(cf, actual.value());
    }
}

TEST(TnTest, OutOfRangeCf) {
    auto result = tn_from_content_format(65535);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), "C-F ID 65535 out of range");
}

TEST(TnTest, OutOfRangeTn) {
    auto below = content_format_from_tn(kTnMin - 1);
    ASSERT_FALSE(below);
    EXPECT_EQ(below.error(), "TN 1668546816 out of range");

    for (uint64_t tn = kTnMin; tn <= kTnMax; ++tn) {
        auto result = content_format_from_tn(tn);
        ASSERT_TRUE(result);
    }

    auto above = content_format_from_tn(kTnMax + 1);
    ASSERT_FALSE(above);
    EXPECT_EQ(above.error(), "TN 1668612096 out of range");
}
