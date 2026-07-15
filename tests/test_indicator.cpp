#include <gtest/gtest.h>

#include "cmw/indicator.hpp"

using cmw::Indicator;
using cmw::IndicatorFlag;

TEST(IndicatorTest, BitOperations) {
    Indicator indicator;
    EXPECT_TRUE(indicator.empty());
    EXPECT_FALSE(indicator.has(IndicatorFlag::AttestationResults));

    indicator.set(IndicatorFlag::AttestationResults);
    EXPECT_TRUE(indicator.has(IndicatorFlag::AttestationResults));

    indicator.clear(IndicatorFlag::AttestationResults);
    EXPECT_TRUE(indicator.empty());

    indicator.set(IndicatorFlag::AttestationResults);
    indicator.toggle(IndicatorFlag::AttestationResults);
    EXPECT_TRUE(indicator.empty());

    indicator.set(IndicatorFlag::AttestationResults);
    indicator.set(IndicatorFlag::ReferenceValues);
    indicator.set(IndicatorFlag::Evidence);
    indicator.set(IndicatorFlag::Endorsements);
    indicator.set(IndicatorFlag::AppraisalPolicy);
    EXPECT_EQ(indicator.to_string(), "appraisal policy, attestation results, endorsements, evidence, reference values");
}
