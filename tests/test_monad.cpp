#include <gtest/gtest.h>

#include "cmw/cmw.hpp"
#include "test_helpers.hpp"

using cmw::CMW;
using cmw::Format;
using cmw::Indicator;
using cmw::IndicatorFlag;
using cmw::Kind;
using cmw::decode;

namespace {

const std::vector<uint8_t> kDeadBeef = {0xde, 0xad, 0xbe, 0xef};

std::vector<uint8_t> bytes_from_text(const char* text) {
    const std::string s(text);
    return std::vector<uint8_t>(s.begin(), s.end());
}

}  // namespace

TEST(MonadTest, DeserializeOk) {
    struct Case {
        std::string name;
        std::vector<uint8_t> input;
        Format expected_format;
        std::string expected_type;
        std::vector<uint8_t> expected_value;
        uint32_t expected_indicator;
    };

    const std::vector<Case> cases = {
        {"JSON array with media type string",
         bytes_from_text(R"(["application/vnd.intel.sgx", "3q2-7w"])"),
         Format::JSONRecord,
         "application/vnd.intel.sgx",
         kDeadBeef,
         0},
        {"JSON array with media type string and indicator",
         bytes_from_text(R"(["application/vnd.intel.sgx", "3q2-7w", 31])"),
         Format::JSONRecord,
         "application/vnd.intel.sgx",
         kDeadBeef,
         31},
        {"CBOR array with CoAP C-F",
         {0x82, 0x19, 0x75, 0x31, 0x44, 0xde, 0xad, 0xbe, 0xef},
         Format::CBORRecord,
         "30001",
         kDeadBeef,
         0},
        {"CBOR array with media type string",
         {0x82, 0x78, 0x19, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e,
          0x2f, 0x76, 0x6e, 0x64, 0x2e, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x2e, 0x73, 0x67, 0x78, 0x44,
          0xde, 0xad, 0xbe, 0xef},
         Format::CBORRecord,
         "application/vnd.intel.sgx",
         kDeadBeef,
         0},
        {"CBOR tag",
         {0xda, 0x63, 0x74, 0x76, 0x32, 0x44, 0xde, 0xad, 0xbe, 0xef},
         Format::CBORTag,
         "29884",
         kDeadBeef,
         0},
    };

    for (const auto& tc : cases) {
        SCOPED_TRACE(tc.name);
        auto decoded = decode(tc.input);
        ASSERT_TRUE(decoded);
        EXPECT_EQ(decoded.value().kind(), Kind::Monad);
        EXPECT_EQ(decoded.value().format(), tc.expected_format);
        EXPECT_TRUE(decoded.value().valid());

        auto type = decoded.value().monad_type();
        ASSERT_TRUE(type);
        EXPECT_EQ(type.value(), tc.expected_type);

        auto value = decoded.value().monad_value();
        ASSERT_TRUE(value);
        EXPECT_EQ(value.value(), tc.expected_value);

        auto indicator = decoded.value().monad_indicator();
        ASSERT_TRUE(indicator);
        EXPECT_EQ(indicator.value().value(), tc.expected_indicator);
    }
}

TEST(MonadTest, MarshalJsonRecord) {
    auto cmw = CMW::create_monad("application/corim+signed", kDeadBeef,
                                   {IndicatorFlag::ReferenceValues, IndicatorFlag::Endorsements,
                                    IndicatorFlag::AppraisalPolicy});
    ASSERT_TRUE(cmw);
    auto encoded = cmw.value().marshal_json();
    ASSERT_TRUE(encoded);
    EXPECT_EQ(std::string(encoded.value().begin(), encoded.value().end()),
              R"(["application/corim+signed","3q2-7w",19])");
}

TEST(MonadTest, MarshalCborRecord) {
    auto cmw = CMW::create_monad(static_cast<uint16_t>(10000), kDeadBeef,
                                   {IndicatorFlag::ReferenceValues, IndicatorFlag::Endorsements,
                                    IndicatorFlag::AppraisalPolicy});
    ASSERT_TRUE(cmw);
    auto encoded = cmw.value().marshal_cbor();
    ASSERT_TRUE(encoded);
    EXPECT_EQ(encoded.value(), std::vector<uint8_t>({0x83, 0x19, 0x27, 0x10, 0x44, 0xde, 0xad, 0xbe,
                                                     0xef, 0x13}));
}

TEST(MonadTest, MarshalCborTag) {
    auto cmw = CMW::create_monad(static_cast<uint16_t>(50000), kDeadBeef);
    ASSERT_TRUE(cmw);
    cmw.value().use_cbor_tag_format();
    auto encoded = cmw.value().marshal_cbor();
    ASSERT_TRUE(encoded);
    EXPECT_EQ(encoded.value(), std::vector<uint8_t>({0xda, 0x63, 0x74, 0xc5, 0x15, 0x44, 0xde, 0xad,
                                                     0xbe, 0xef}));
}

TEST(MonadTest, SpecJsonRecordExample) {
    auto decoded = decode(bytes_from_text(R"(["application/vnd.example.rats-conceptual-msg", "I0faVQ"])"));
    ASSERT_TRUE(decoded);
    auto value = decoded.value().monad_value();
    ASSERT_TRUE(value);
    EXPECT_EQ(value.value(), std::vector<uint8_t>({0x23, 0x47, 0xda, 0x55}));
}

TEST(MonadTest, SpecCborRecordExample) {
    const auto input = std::vector<uint8_t>{0x82, 0x19, 0xfd, 0xe7, 0x44, 0x23, 0x47, 0xda, 0x55};
    auto decoded = decode(input);
    ASSERT_TRUE(decoded);
    auto type = decoded.value().monad_type();
    ASSERT_TRUE(type);
    EXPECT_EQ(type.value(), "64999");
    auto value = decoded.value().monad_value();
    ASSERT_TRUE(value);
    EXPECT_EQ(value.value(), std::vector<uint8_t>({0x23, 0x47, 0xda, 0x55}));
}

TEST(MonadTest, SpecCborTagExample) {
    const auto input = std::vector<uint8_t>{0xda, 0x63, 0x74, 0xff, 0xe6, 0x44, 0x23, 0x47, 0xda, 0x55};
    auto decoded = decode(input);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded.value().format(), Format::CBORTag);
    auto value = decoded.value().monad_value();
    ASSERT_TRUE(value);
    EXPECT_EQ(value.value(), std::vector<uint8_t>({0x23, 0x47, 0xda, 0x55}));
}

TEST(MonadTest, SpecCborRecordWithIndicator) {
    const auto input = hex_decode(R"(
        83
        74 6170706c69636174696f6e2f72696d2b636f7365
        4a d28440a044d901f5a040
        03
    )");
    auto decoded = decode(input);
    ASSERT_TRUE(decoded);
    auto type = decoded.value().monad_type();
    ASSERT_TRUE(type);
    EXPECT_EQ(type.value(), "application/rim+cose");
    auto indicator = decoded.value().monad_indicator();
    ASSERT_TRUE(indicator);
    EXPECT_EQ(indicator.value().value(), 3u);
}
