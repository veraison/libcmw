#include "cmw/cbor_internal.hpp"

#include <cstring>

#include "cmw/cfmap.hpp"
#include "cmw/qcbor_util.hpp"
#include "cmw/util.hpp"

namespace cmw::cbor_internal {

namespace {

Result<void> set_type_from_item(const QCBORItem& item, Type& type) {
    if (item.uDataType == QCBOR_TYPE_TEXT_STRING) {
        std::string mt(reinterpret_cast<const char*>(item.val.string.ptr), item.val.string.len);
        return type.set_media_type(mt);
    }
    if (qcbor_util::is_uint_item(item)) {
        const uint64_t value = qcbor_util::uint_item_value(item);
        if (value > UINT16_MAX) {
            return Result<void>::err("cannot unmarshal " + std::to_string(value) + " into uint16");
        }
        return type.set_content_format(static_cast<uint16_t>(value));
    }
    return Result<void>::err("expecting string or uint16");
}

Result<void> set_value_from_item(const QCBORItem& item, std::vector<uint8_t>& value) {
    if (item.uDataType != QCBOR_TYPE_BYTE_STRING) {
        return Result<void>::err("cannot decode value");
    }
    if (item.val.string.len == 0) {
        return Result<void>::err("empty value");
    }
    const auto* bytes = static_cast<const uint8_t*>(item.val.string.ptr);
    value.assign(bytes, bytes + item.val.string.len);
    return Result<void>::ok();
}

Result<void> decode_record_items(QCBORDecodeContext& ctx, Type& type, std::vector<uint8_t>& value,
                                 Indicator& indicator, Format& format, int count) {
    if (count < 2 || count > 3) {
        return Result<void>::err("wrong number of entries (" + std::to_string(count) +
                                 ") in the CMW record");
    }

    QCBORItem item;
    QCBORError err = QCBORDecode_GetNext(&ctx, &item);
    if (err != QCBOR_SUCCESS) {
        return Result<void>::err("decoding record");
    }
    auto type_result = set_type_from_item(item, type);
    if (!type_result) {
        return Result<void>::err("decoding record: unmarshaling type: " + type_result.error());
    }

    err = QCBORDecode_GetNext(&ctx, &item);
    if (err != QCBOR_SUCCESS) {
        return Result<void>::err("decoding record: unmarshaling value");
    }
    auto value_result = set_value_from_item(item, value);
    if (!value_result) {
        return Result<void>::err("decoding record: unmarshaling value: " + value_result.error());
    }

    if (count == 3) {
        err = QCBORDecode_GetNext(&ctx, &item);
        if (err != QCBOR_SUCCESS || !qcbor_util::is_uint_item(item)) {
            return Result<void>::err("decoding record: unmarshaling indicator");
        }
        const uint64_t ind = qcbor_util::uint_item_value(item);
        if (ind == 0) {
            return Result<void>::err("indicator field, if present, MUST be non-zero");
        }
        indicator = Indicator(static_cast<uint32_t>(ind));
    } else {
        indicator = Indicator(kIndicatorNone);
    }

    format = Format::CBORRecord;
    return Result<void>::ok();
}

Result<void> decode_tag_item(QCBORDecodeContext& ctx, const QCBORItem& item, Type& type,
                             std::vector<uint8_t>& value, Indicator& indicator, Format& format) {
    if (!qcbor_util::item_has_tag(item)) {
        return Result<void>::err("decoding tag");
    }
    if (item.uDataType != QCBOR_TYPE_BYTE_STRING) {
        return Result<void>::err("decoding tag: unmarshal CMW CBOR Tag bstr-wrapped value");
    }
    if (item.val.string.len == 0) {
        return Result<void>::err("decoding tag: unmarshal CMW CBOR Tag bstr-wrapped value: EOF");
    }

    auto tag = qcbor_util::item_tag_number(ctx, item);
    if (!tag) {
        return Result<void>::err("decoding tag: " + tag.error());
    }
    auto type_result = type.set_tag_number(tag.value());
    if (!type_result) {
        return Result<void>::err("decoding tag: setting type: " + type_result.error());
    }

    auto value_result = set_value_from_item(item, value);
    if (!value_result) {
        return Result<void>::err("decoding tag: setting value: " + value_result.error());
    }

    indicator = Indicator(kIndicatorNone);
    format = Format::CBORTag;
    return Result<void>::ok();
}

CollectionKey key_from_item(const QCBORItem& item) {
#ifndef QCBOR_DISABLE_NON_INTEGER_LABELS
    if (item.uLabelType == QCBOR_TYPE_TEXT_STRING) {
        return CollectionKey{std::string(reinterpret_cast<const char*>(item.label.string.ptr),
                                        item.label.string.len)};
    }
    if (item.uLabelType == QCBOR_TYPE_UINT64 || item.uLabelType == QCBOR_TYPE_INT64) {
        if (item.uLabelType == QCBOR_TYPE_INT64 && item.label.int64 < 0) {
            return CollectionKey{static_cast<uint64_t>(item.label.int64)};
        }
        if (item.uLabelType == QCBOR_TYPE_INT64) {
            return CollectionKey{static_cast<uint64_t>(item.label.int64)};
        }
        return CollectionKey{item.label.uint64};
    }
#endif
    return CollectionKey{std::string{}};
}

}  // namespace

Result<void> decode_monad_from_map_entry(QCBORDecodeContext& ctx, const QCBORItem& entry, CMW& out) {
    if (entry.uDataType == QCBOR_TYPE_ARRAY) {
        auto result = decode_record_items(ctx, out.monad_.type, out.monad_.value,
                                          out.monad_.indicator, out.monad_.format,
                                          entry.val.uCount);
        if (!result) {
            return result;
        }
        out.kind_ = Kind::Monad;
        out.format_ = out.monad_.format;
        return Result<void>::ok();
    }
    if (qcbor_util::item_has_tag(entry)) {
        auto result = decode_tag_item(ctx, entry, out.monad_.type, out.monad_.value,
                                      out.monad_.indicator, out.monad_.format);
        if (!result) {
            return result;
        }
        out.kind_ = Kind::Monad;
        out.format_ = out.monad_.format;
        return Result<void>::ok();
    }
    return Result<void>::err("unsupported nested monad type");
}

Result<void> decode_collection_from_context(QCBORDecodeContext& ctx, const QCBORItem& map_item,
                                            CMW& out) {
    if (map_item.uDataType != QCBOR_TYPE_MAP) {
        return Result<void>::err("unmarshaling CBOR collection");
    }

    out.collection_.items.clear();
    out.collection_.cmwc_t.clear();

    const uint8_t map_nest = map_item.uNestingLevel;
    QCBORItem item;
    QCBORError err = QCBOR_SUCCESS;

    while ((err = QCBORDecode_GetNext(&ctx, &item)) == QCBOR_SUCCESS) {
        if (item.uDataType == QCBOR_TYPE_BREAK) {
            break;
        }
        if (item.uNestingLevel <= map_nest && item.uLabelType == QCBOR_TYPE_NONE) {
            break;
        }

        const CollectionKey key = key_from_item(item);
        if (std::holds_alternative<std::string>(key) && std::get<std::string>(key).empty()) {
            return Result<void>::err("unknown collection key type");
        }

        if (std::holds_alternative<std::string>(key) &&
            std::get<std::string>(key) == "__cmwc_t") {
            if (item.uDataType != QCBOR_TYPE_TEXT_STRING) {
                return Result<void>::err("extracting CBOR collection type");
            }
            out.collection_.cmwc_t.assign(reinterpret_cast<const char*>(item.val.string.ptr),
                                          item.val.string.len);
            if (!validate_collection_type(out.collection_.cmwc_t)) {
                return Result<void>::err("checking CBOR collection type: invalid collection type");
            }
            continue;
        }

        CMW nested;
        if (item.uDataType == QCBOR_TYPE_MAP) {
            auto result = decode_collection_from_context(ctx, item, nested);
            if (!result) {
                return result;
            }
        } else {
            auto result = decode_monad_from_map_entry(ctx, item, nested);
            if (!result) {
                return result;
            }
        }

        out.collection_.items.emplace(key, std::move(nested));
    }

    if (err != QCBOR_SUCCESS && err != QCBOR_ERR_NO_MORE_ITEMS) {
        return Result<void>::err("unmarshaling CBOR collection: unexpected EOF");
    }

    out.collection_.format = Format::CBORCollection;
    out.kind_ = Kind::Collection;
    out.format_ = Format::CBORCollection;
    return Result<void>::ok();
}

}  // namespace cmw::cbor_internal
