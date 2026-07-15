#include <gtest/gtest.h>

#include "cmw/cmw.hpp"

using cmw::CMW;
using cmw::Format;
using cmw::IndicatorFlag;
using cmw::Kind;
using cmw::decode;

TEST(CmwTest, CreateMonad) {
    auto cmw = CMW::create_monad("text/plain; charset=utf-8", std::vector<uint8_t>{0xff},
                                 {IndicatorFlag::Evidence});
    ASSERT_TRUE(cmw);
    EXPECT_EQ(cmw.value().kind(), Kind::Monad);
    auto type = cmw.value().monad_type();
    ASSERT_TRUE(type);
    EXPECT_EQ(type.value(), "text/plain; charset=utf-8");
    auto value = cmw.value().monad_value();
    ASSERT_TRUE(value);
    EXPECT_EQ(value.value(), std::vector<uint8_t>{0xff});
    auto indicator = cmw.value().monad_indicator();
    ASSERT_TRUE(indicator);
    EXPECT_TRUE(indicator.value().has(IndicatorFlag::Evidence));
}

TEST(CmwTest, CreateCollection) {
    const std::string ctyp = "tag:example.com,2024:composite-attester";
    auto cmw = CMW::create_collection(ctyp);
    ASSERT_TRUE(cmw);
    EXPECT_EQ(cmw.value().kind(), Kind::Collection);
    auto type = cmw.value().collection_type();
    ASSERT_TRUE(type);
    EXPECT_EQ(type.value(), ctyp);
    auto meta = cmw.value().collection_meta();
    ASSERT_TRUE(meta);
    EXPECT_TRUE(meta.value().empty());
}

TEST(CmwTest, RoundtripJsonCollection) {
    const std::string input = R"({
  "bretwaldadom": [
    "application/eat-ucs+cbor",
    "oQo"
  ],
  "__cmwc_t": "tag:ietf.org,2024:X",
  "json-raw": [
    "application/vnd.my.ref-val",
    "e30K"
  ],
  "murmurless": {
    "__cmwc_t": "tag:ietf.org,2024:Y",
    "polyscopic": [
      "application/eat-ucs+json",
      "eyJlYXRfbm9uY2UiOiAuLi59",
      8
    ]
  },
  "photoelectrograph": [
    "application/eat-ucs+cbor",
    "gngY",
    3
  ]
})";

    auto decoded = decode(std::vector<uint8_t>(input.begin(), input.end()));
    ASSERT_TRUE(decoded);
    ASSERT_TRUE(decoded.value().valid());

    auto encoded = decoded.value().marshal_json();
    ASSERT_TRUE(encoded);

    const std::string expected =
        R"({"__cmwc_t":"tag:ietf.org,2024:X","bretwaldadom":["application/eat-ucs+cbor","oQo"],"json-raw":["application/vnd.my.ref-val","e30K"],"murmurless":{"__cmwc_t":"tag:ietf.org,2024:Y","polyscopic":["application/eat-ucs+json","eyJlYXRfbm9uY2UiOiAuLi59",8]},"photoelectrograph":["application/eat-ucs+cbor","gngY",3]})";
    EXPECT_EQ(std::string(encoded.value().begin(), encoded.value().end()), expected);
}

TEST(CmwTest, BadMediaTypeRejected) {
    auto cmw = CMW::create_monad("application/ vnd.example.evidence", std::vector<uint8_t>{0x00});
    ASSERT_FALSE(cmw);
}

TEST(CmwTest, EmptyValueRejected) {
    auto cmw = CMW::create_monad("application/vnd.example.evidence", {});
    ASSERT_FALSE(cmw);
    EXPECT_EQ(cmw.error(), "empty value");
}
