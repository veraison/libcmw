#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <qcbor/qcbor.h>

#include "cmw/indicator.hpp"
#include "cmw/result.hpp"
#include "cmw/type.hpp"

namespace cmw {

enum class Kind {
    Unknown,
    Monad,
    Collection,
};

enum class Format {
    Unknown,
    JSONRecord,
    JSONCollection,
    CBORRecord,
    CBORCollection,
    CBORTag,
};

std::string kind_to_string(Kind kind);
std::string format_to_string(Format format);

using CollectionKey = std::variant<std::string, uint64_t>;

struct CollectionKeyCompare {
    bool operator()(const CollectionKey& lhs, const CollectionKey& rhs) const;
};

struct Meta {
    CollectionKey key;
    Kind kind = Kind::Unknown;
};

class CMW;

namespace cbor_internal {
Result<void> decode_monad_from_map_entry(QCBORDecodeContext& ctx, const QCBORItem& entry, CMW& out);
Result<void> decode_collection_from_context(QCBORDecodeContext& ctx, const QCBORItem& map_item,
                                            CMW& out);
}  // namespace cbor_internal

class CMW {
public:
    struct Monad {
        Type type;
        std::vector<uint8_t> value;
        Indicator indicator;
        Format format = Format::Unknown;

        Result<void> validate() const;
        Result<std::vector<uint8_t>> marshal_json() const;
        Result<std::vector<uint8_t>> marshal_cbor() const;
        Result<void> unmarshal_json(const uint8_t* data, size_t len);
        Result<void> unmarshal_cbor(const uint8_t* data, size_t len);
    };

    struct Collection {
        std::map<CollectionKey, CMW, CollectionKeyCompare> items;
        std::string cmwc_t;
        Format format = Format::Unknown;

        Result<void> validate() const;
        Result<std::vector<uint8_t>> marshal_json() const;
        Result<std::vector<uint8_t>> marshal_cbor() const;
        Result<void> unmarshal_json(const uint8_t* data, size_t len);
        Result<void> unmarshal_cbor(const uint8_t* data, size_t len);
    };

    static Result<CMW> create_monad(const std::string& media_type,
                                    const std::vector<uint8_t>& value,
                                    const std::vector<IndicatorFlag>& indicators = {});
    static Result<CMW> create_monad(uint16_t content_format,
                                    const std::vector<uint8_t>& value,
                                    const std::vector<IndicatorFlag>& indicators = {});
    static Result<CMW> create_collection(const std::string& cmwc_t = "");

    Kind kind() const { return kind_; }
    Format format() const { return format_; }

    Result<std::string> monad_type() const;
    Result<std::vector<uint8_t>> monad_value() const;
    Result<Indicator> monad_indicator() const;
    void use_cbor_tag_format();

    Result<std::string> collection_type() const;
    Result<void> add_collection_item(const CollectionKey& key, const CMW& node);
    Result<CMW> get_collection_item(const CollectionKey& key) const;
    Result<std::vector<Meta>> collection_meta() const;

    Result<void> valid() const;

    Result<std::vector<uint8_t>> marshal_json() const;
    Result<std::vector<uint8_t>> marshal_cbor() const;
    Result<void> unmarshal_json(const uint8_t* data, size_t len);
    Result<void> unmarshal_json(const std::vector<uint8_t>& data) {
        return unmarshal_json(data.data(), data.size());
    }
    Result<void> unmarshal_cbor(const uint8_t* data, size_t len);
    Result<void> unmarshal_cbor(const std::vector<uint8_t>& data) {
        return unmarshal_cbor(data.data(), data.size());
    }

private:
    Kind kind_ = Kind::Unknown;
    Format format_ = Format::Unknown;
    Monad monad_;
    Collection collection_;

    static void apply_indicators(Indicator& target, const std::vector<IndicatorFlag>& indicators);

    friend Result<void> cbor_internal::decode_monad_from_map_entry(QCBORDecodeContext&,
                                                                   const QCBORItem&, CMW&);
    friend Result<void> cbor_internal::decode_collection_from_context(QCBORDecodeContext&,
                                                                      const QCBORItem&, CMW&);
};

Result<CMW> decode(const uint8_t* data, size_t len);
inline Result<CMW> decode(const std::vector<uint8_t>& data) {
    return decode(data.data(), data.size());
}

Format sniff(const uint8_t* data, size_t len);
inline Format sniff(const std::vector<uint8_t>& data) {
    return sniff(data.data(), data.size());
}

bool validate_collection_type(const std::string& cmwc_t);
Result<void> validate_collection_key(const CollectionKey& key);

}  // namespace cmw
