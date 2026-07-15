#include <gtest/gtest.h>

#include "cmw/cmw.hpp"
#include "test_helpers.hpp"

using cmw::CMW;
using cmw::CollectionKey;
using cmw::Format;
using cmw::Indicator;
using cmw::IndicatorFlag;
using cmw::Kind;
using cmw::decode;
using cmw::validate_collection_key;
using cmw::validate_collection_type;

TEST(CollectionTest, JsonDeserializeOk) {
    const auto tv = read_file_bytes(testdata_path("collection-ok.json"));
    CMW actual;
    ASSERT_TRUE(actual.unmarshal_json(tv));

    EXPECT_EQ(actual.format(), Format::JSONCollection);

    auto a = actual.get_collection_item(CollectionKey{std::string{"a"}});
    ASSERT_TRUE(a);
    EXPECT_EQ(a.value().format(), Format::JSONRecord);
    EXPECT_EQ(a.value().kind(), Kind::Monad);
    auto a_type = a.value().monad_type();
    ASSERT_TRUE(a_type);
    EXPECT_EQ(a_type.value(), "application/vnd.a");
    auto a_value = a.value().monad_value();
    ASSERT_TRUE(a_value);
    EXPECT_EQ(a_value.value(), std::vector<uint8_t>{0x61});

    auto b = actual.get_collection_item(CollectionKey{std::string{"b"}});
    ASSERT_TRUE(b);
    EXPECT_EQ(b.value().format(), Format::JSONRecord);
    auto b_type = b.value().monad_type();
    ASSERT_TRUE(b_type);
    EXPECT_EQ(b_type.value(), "application/vnd.b");
}

TEST(CollectionTest, JsonSerializeOk) {
    auto collection = CMW::create_collection("");
    ASSERT_TRUE(collection);

    auto a = CMW::create_monad("application/vnd.a", std::vector<uint8_t>{0x61});
    ASSERT_TRUE(a);
    ASSERT_TRUE(collection.value().add_collection_item(CollectionKey{std::string{"a"}}, a.value()));

    auto b = CMW::create_monad("application/vnd.b", std::vector<uint8_t>{0x62});
    ASSERT_TRUE(b);
    ASSERT_TRUE(collection.value().add_collection_item(CollectionKey{std::string{"b"}}, b.value()));

    auto actual = collection.value().marshal_json();
    ASSERT_TRUE(actual);

    CMW roundtrip;
    ASSERT_TRUE(roundtrip.unmarshal_json(actual.value()));
    auto item_a = roundtrip.get_collection_item(CollectionKey{std::string{"a"}});
    ASSERT_TRUE(item_a);
    auto item_b = roundtrip.get_collection_item(CollectionKey{std::string{"b"}});
    ASSERT_TRUE(item_b);
}

TEST(CollectionTest, CborDeserializeOk) {
    const auto tv = read_file_bytes(testdata_path("collection-cbor-ok.cbor"));
    CMW actual;
    ASSERT_TRUE(actual.unmarshal_cbor(tv));
    EXPECT_EQ(actual.format(), Format::CBORCollection);

    auto one = actual.get_collection_item(CollectionKey{uint64_t{1}});
    ASSERT_TRUE(one);
    EXPECT_EQ(one.value().kind(), Kind::Monad);
    EXPECT_EQ(one.value().format(), Format::CBORRecord);
    auto one_type = one.value().monad_type();
    ASSERT_TRUE(one_type);
    EXPECT_EQ(one_type.value(), "application/signed-corim+cbor");
    auto one_indicator = one.value().monad_indicator();
    ASSERT_TRUE(one_indicator);
    EXPECT_EQ(one_indicator.value().value(), 3u);

    auto two = actual.get_collection_item(CollectionKey{uint64_t{2}});
    ASSERT_TRUE(two);
    EXPECT_EQ(two.value().format(), Format::CBORTag);
    auto two_type = two.value().monad_type();
    ASSERT_TRUE(two_type);
    EXPECT_EQ(two_type.value(), "29884");

    auto s = actual.get_collection_item(CollectionKey{std::string{"s"}});
    ASSERT_TRUE(s);
    EXPECT_EQ(s.value().format(), Format::CBORRecord);
    auto s_type = s.value().monad_type();
    ASSERT_TRUE(s_type);
    EXPECT_EQ(s_type.value(), "30001");
}

TEST(CollectionTest, CborSerializeOk) {
    const auto expected = read_file_bytes(testdata_path("collection-cbor-ok-2.cbor"));

    auto item = CMW::create_monad("application/vnd.1", std::vector<uint8_t>{0xde, 0xad, 0xbe, 0xef});
    ASSERT_TRUE(item);

    auto collection = CMW::create_collection("");
    ASSERT_TRUE(collection);
    ASSERT_TRUE(collection.value().add_collection_item(CollectionKey{uint64_t{1}}, item.value()));

    auto actual = collection.value().marshal_cbor();
    ASSERT_TRUE(actual);
    EXPECT_EQ(actual.value(), expected);
}

TEST(CollectionTest, DecodeJsonOk) {
    const auto tv = read_file_bytes(testdata_path("collection-ok.json"));
    auto decoded = decode(tv);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded.value().kind(), Kind::Collection);
    EXPECT_EQ(decoded.value().format(), Format::JSONCollection);
}

TEST(CollectionTest, DecodeCborOk) {
    const auto tv = read_file_bytes(testdata_path("collection-cbor-ok.cbor"));
    auto decoded = decode(tv);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded.value().kind(), Kind::Collection);
    EXPECT_EQ(decoded.value().format(), Format::CBORCollection);
}

TEST(CollectionTest, ValidateCollectionType) {
    EXPECT_TRUE(validate_collection_type("tag:example.com,2024:composite-attester"));
    EXPECT_TRUE(validate_collection_type("urn:ietf:rfc:rfc9999"));
    EXPECT_TRUE(validate_collection_type("http://www.ietf.org/rfc/rfc2396.txt"));
    EXPECT_TRUE(validate_collection_type("1.2.3.4"));
    EXPECT_FALSE(validate_collection_type("a/b/c"));
    EXPECT_FALSE(validate_collection_type(".2.3.4"));
}

TEST(CollectionTest, ValidateCollectionKey) {
    EXPECT_TRUE(validate_collection_key(CollectionKey{std::string{"my-label"}}));
    EXPECT_TRUE(validate_collection_key(CollectionKey{uint64_t{1}}));
    EXPECT_FALSE(validate_collection_key(CollectionKey{std::string{""}}));
    EXPECT_FALSE(validate_collection_key(CollectionKey{std::string{"   "}}));
    EXPECT_FALSE(validate_collection_key(CollectionKey{std::string{"__cmwc_t"}}));
}
