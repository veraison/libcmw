#include "cmw/cmw.hpp"

#include <algorithm>
#include <sstream>

#include "cmw/util.hpp"

namespace cmw {

bool CollectionKeyCompare::operator()(const CollectionKey& lhs,
                                      const CollectionKey& rhs) const {
    return collection_key_sort_string(lhs) < collection_key_sort_string(rhs);
}

std::string kind_to_string(Kind kind) {
    switch (kind) {
        case Kind::Collection:
            return "collection";
        case Kind::Monad:
            return "monad";
        case Kind::Unknown:
        default:
            return "unknown";
    }
}

std::string format_to_string(Format format) {
    switch (format) {
        case Format::JSONRecord:
            return "JSON record";
        case Format::JSONCollection:
            return "JSON collection";
        case Format::CBORRecord:
            return "CBOR record";
        case Format::CBORCollection:
            return "CBOR collection";
        case Format::CBORTag:
            return "CBOR tag";
        case Format::Unknown:
        default:
            return "unknown";
    }
}

void CMW::apply_indicators(Indicator& target, const std::vector<IndicatorFlag>& indicators) {
    for (const auto flag : indicators) {
        target.set(flag);
    }
}

Result<CMW> CMW::create_monad(const std::string& media_type, const std::vector<uint8_t>& value,
                              const std::vector<IndicatorFlag>& indicators) {
    if (value.empty()) {
        return Result<CMW>::err("empty value");
    }

    CMW cmw;
    auto type_result = cmw.monad_.type.set_media_type(media_type);
    if (!type_result) {
        return Result<CMW>::err(type_result.error());
    }
    cmw.monad_.value = value;
    apply_indicators(cmw.monad_.indicator, indicators);
    cmw.kind_ = Kind::Monad;
    return Result<CMW>::ok(std::move(cmw));
}

Result<CMW> CMW::create_monad(uint16_t content_format, const std::vector<uint8_t>& value,
                              const std::vector<IndicatorFlag>& indicators) {
    if (value.empty()) {
        return Result<CMW>::err("empty value");
    }

    CMW cmw;
    auto type_result = cmw.monad_.type.set_content_format(content_format);
    if (!type_result) {
        return Result<CMW>::err(type_result.error());
    }
    cmw.monad_.value = value;
    apply_indicators(cmw.monad_.indicator, indicators);
    cmw.kind_ = Kind::Monad;
    return Result<CMW>::ok(std::move(cmw));
}

Result<CMW> CMW::create_collection(const std::string& cmwc_t) {
    if (!cmwc_t.empty() && !validate_collection_type(cmwc_t)) {
        return Result<CMW>::err("invalid collection type: \"" + cmwc_t +
                                "\".  URI is not absolute");
    }

    CMW cmw;
    cmw.collection_.cmwc_t = cmwc_t;
    cmw.kind_ = Kind::Collection;
    return Result<CMW>::ok(std::move(cmw));
}

Result<std::string> CMW::monad_type() const {
    if (kind_ != Kind::Monad) {
        return Result<std::string>::err("want monad, got \"" + kind_to_string(kind_) + "\"");
    }
    return Result<std::string>::ok(monad_.type.to_string());
}

Result<std::vector<uint8_t>> CMW::monad_value() const {
    if (kind_ != Kind::Monad) {
        return Result<std::vector<uint8_t>>::err("want monad, got \"" + kind_to_string(kind_) +
                                                 "\"");
    }
    return Result<std::vector<uint8_t>>::ok(monad_.value);
}

Result<Indicator> CMW::monad_indicator() const {
    if (kind_ != Kind::Monad) {
        return Result<Indicator>::err("want monad, got \"" + kind_to_string(kind_) + "\"");
    }
    return Result<Indicator>::ok(monad_.indicator);
}

void CMW::use_cbor_tag_format() {
    monad_.format = Format::CBORTag;
}

Result<std::string> CMW::collection_type() const {
    if (kind_ != Kind::Collection) {
        return Result<std::string>::err("want collection, got \"" + kind_to_string(kind_) + "\"");
    }
    return Result<std::string>::ok(collection_.cmwc_t);
}

Result<void> CMW::add_collection_item(const CollectionKey& key, const CMW& node) {
    if (kind_ != Kind::Collection) {
        return Result<void>::err("want collection, got \"" + kind_to_string(kind_) + "\"");
    }

    auto key_result = validate_collection_key(key);
    if (!key_result) {
        return Result<void>::err("invalid key: " + key_result.error());
    }

    collection_.items[key] = node;
    return Result<void>::ok();
}

Result<CMW> CMW::get_collection_item(const CollectionKey& key) const {
    if (kind_ != Kind::Collection) {
        return Result<CMW>::err("want collection, got \"" + kind_to_string(kind_) + "\"");
    }

    const auto it = collection_.items.find(key);
    if (it == collection_.items.end()) {
        std::ostringstream oss;
        oss << "item not found for key ";
        if (std::holds_alternative<std::string>(key)) {
            oss << '"' << std::get<std::string>(key) << '"';
        } else {
            oss << std::get<uint64_t>(key);
        }
        return Result<CMW>::err(oss.str());
    }
    return Result<CMW>::ok(it->second);
}

Result<std::vector<Meta>> CMW::collection_meta() const {
    if (kind_ != Kind::Collection) {
        return Result<std::vector<Meta>>::err("want collection, got \"" + kind_to_string(kind_) +
                                              "\"");
    }

    std::vector<Meta> meta;
    meta.reserve(collection_.items.size());
    for (const auto& [key, value] : collection_.items) {
        meta.push_back(Meta{key, value.kind()});
    }

    std::sort(meta.begin(), meta.end(), [](const Meta& a, const Meta& b) {
        return collection_key_sort_string(a.key) < collection_key_sort_string(b.key);
    });
    return Result<std::vector<Meta>>::ok(std::move(meta));
}

Result<void> CMW::valid() const {
    switch (kind_) {
        case Kind::Monad:
            return monad_.validate();
        case Kind::Collection:
            return collection_.validate();
        default:
            return Result<void>::err("unknown kind: " + kind_to_string(kind_));
    }
}

Result<std::vector<uint8_t>> CMW::marshal_json() const {
    switch (kind_) {
        case Kind::Monad:
            return monad_.marshal_json();
        case Kind::Collection:
            return collection_.marshal_json();
        default:
            return Result<std::vector<uint8_t>>::err("unknown CMW kind");
    }
}

Result<std::vector<uint8_t>> CMW::marshal_cbor() const {
    switch (kind_) {
        case Kind::Monad:
            return monad_.marshal_cbor();
        case Kind::Collection:
            return collection_.marshal_cbor();
        default:
            return Result<std::vector<uint8_t>>::err("unknown CMW kind");
    }
}

Result<void> CMW::unmarshal_json(const uint8_t* data, size_t len) {
    if (len == 0) {
        return Result<void>::err("empty buffer");
    }

    const uint8_t start = data[0];
    if (start_json_record(start)) {
        auto result = monad_.unmarshal_json(data, len);
        if (!result) {
            return result;
        }
        kind_ = Kind::Monad;
        format_ = monad_.format;
        return Result<void>::ok();
    }
    if (start_json_collection(start)) {
        auto result = collection_.unmarshal_json(data, len);
        if (!result) {
            return result;
        }
        kind_ = Kind::Collection;
        format_ = collection_.format;
        return Result<void>::ok();
    }
    return Result<void>::err("want JSON object or JSON array start symbols, got: 0x" +
                            std::to_string(start));
}

Result<void> CMW::unmarshal_cbor(const uint8_t* data, size_t len) {
    if (len == 0) {
        return Result<void>::err("empty buffer");
    }

    const uint8_t start = data[0];
    if (start_cbor_record(start) || start_cbor_tag(start)) {
        auto result = monad_.unmarshal_cbor(data, len);
        if (!result) {
            return result;
        }
        kind_ = Kind::Monad;
        format_ = monad_.format;
        return Result<void>::ok();
    }
    if (start_cbor_collection(start)) {
        auto result = collection_.unmarshal_cbor(data, len);
        if (!result) {
            return result;
        }
        kind_ = Kind::Collection;
        format_ = collection_.format;
        return Result<void>::ok();
    }
    return Result<void>::err("want CBOR map, CBOR array or CBOR Tag start symbols, got: 0x" +
                            std::to_string(start));
}

Result<CMW> decode(const uint8_t* data, size_t len) {
    CMW cmw;
    if (len == 0) {
        return Result<CMW>::err("empty buffer");
    }

    const uint8_t start = data[0];
    if (start_cbor_collection(start) || start_cbor_record(start) || start_cbor_tag(start)) {
        auto result = cmw.unmarshal_cbor(data, len);
        if (!result) {
            return Result<CMW>::err(result.error());
        }
        return Result<CMW>::ok(std::move(cmw));
    }
    if (start_json_record(start) || start_json_collection(start)) {
        auto result = cmw.unmarshal_json(data, len);
        if (!result) {
            return Result<CMW>::err(result.error());
        }
        return Result<CMW>::ok(std::move(cmw));
    }
    return Result<CMW>::err("unknown start symbol for CMW");
}

}  // namespace cmw
