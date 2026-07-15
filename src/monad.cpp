#include "cmw/cmw.hpp"

#include <cJSON.h>
#include <qcbor/qcbor.h>

#include <cstring>

#include "cmw/qcbor_util.hpp"
#include "cmw/tn.hpp"
#include "cmw/util.hpp"

namespace cmw {

namespace {

constexpr size_t kEncodeBufferSize = 4096;

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

Result<void> decode_record_items(QCBORDecodeContext& ctx, CMW::Monad& monad, int count) {
    if (count < 2 || count > 3) {
        return Result<void>::err("wrong number of entries (" + std::to_string(count) +
                                 ") in the CMW record");
    }

    QCBORItem item;
    QCBORError err = QCBORDecode_GetNext(&ctx, &item);
    if (err != QCBOR_SUCCESS) {
        return Result<void>::err("decoding record");
    }
    auto type_result = set_type_from_item(item, monad.type);
    if (!type_result) {
        return Result<void>::err("decoding record: unmarshaling type: " + type_result.error());
    }

    err = QCBORDecode_GetNext(&ctx, &item);
    if (err != QCBOR_SUCCESS) {
        return Result<void>::err("decoding record: unmarshaling value");
    }
    auto value_result = set_value_from_item(item, monad.value);
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
        monad.indicator = Indicator(static_cast<uint32_t>(ind));
    } else {
        monad.indicator = Indicator(kIndicatorNone);
    }

    monad.format = Format::CBORRecord;
    return Result<void>::ok();
}

Result<void> decode_record_cbor(QCBORDecodeContext& ctx, CMW::Monad& monad) {
    QCBORItem array_item;
    QCBORError err = QCBORDecode_GetNext(&ctx, &array_item);
    if (err != QCBOR_SUCCESS || array_item.uDataType != QCBOR_TYPE_ARRAY) {
        return Result<void>::err("decoding record");
    }
    return decode_record_items(ctx, monad, array_item.val.uCount);
}

Result<void> decode_tag_item(QCBORDecodeContext& ctx, const QCBORItem& item, CMW::Monad& monad) {
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
    auto type_result = monad.type.set_tag_number(tag.value());
    if (!type_result) {
        return Result<void>::err("decoding tag: setting type: " + type_result.error());
    }

    auto value_result = set_value_from_item(item, monad.value);
    if (!value_result) {
        return Result<void>::err("decoding tag: setting value: " + value_result.error());
    }

    monad.indicator = Indicator(kIndicatorNone);
    monad.format = Format::CBORTag;
    return Result<void>::ok();
}

Result<void> decode_record_json(const uint8_t* data, size_t len, CMW::Monad& monad) {
    const std::string raw(reinterpret_cast<const char*>(data), len);
    cJSON* array = cJSON_Parse(raw.c_str());
    if (array == nullptr) {
        return Result<void>::err("unexpected end of JSON input");
    }
    if (!cJSON_IsArray(array)) {
        cJSON_Delete(array);
        return Result<void>::err("wrong number of entries (0) in the CMW record");
    }

    const int count = cJSON_GetArraySize(array);
    if (count < 2 || count > 3) {
        cJSON_Delete(array);
        return Result<void>::err("wrong number of entries (" + std::to_string(count) +
                                 ") in the CMW record");
    }

    cJSON* type_item = cJSON_GetArrayItem(array, 0);
    char* type_json = cJSON_PrintUnformatted(type_item);
    if (type_json == nullptr) {
        cJSON_Delete(array);
        return Result<void>::err("unmarshaling type");
    }
    auto type_result = monad.type.decode_json(reinterpret_cast<const uint8_t*>(type_json),
                                              std::strlen(type_json));
    cJSON_free(type_json);
    if (!type_result) {
        cJSON_Delete(array);
        return Result<void>::err("unmarshaling type: " + type_result.error());
    }

    cJSON* value_item = cJSON_GetArrayItem(array, 1);
    if (!cJSON_IsString(value_item)) {
        cJSON_Delete(array);
        return Result<void>::err(
            "unmarshaling value: cannot base64 url-safe decode: illegal base64 data at input "
            "byte 0");
    }
    auto value_result = base64url_decode(value_item->valuestring);
    if (!value_result) {
        cJSON_Delete(array);
        return Result<void>::err("unmarshaling value: cannot base64 url-safe decode: " +
                                 value_result.error());
    }
    if (value_result.value().empty()) {
        cJSON_Delete(array);
        return Result<void>::err("empty value");
    }
    monad.value = std::move(value_result.value());

    if (count == 3) {
        cJSON* ind_item = cJSON_GetArrayItem(array, 2);
        if (!cJSON_IsNumber(ind_item)) {
            cJSON_Delete(array);
            return Result<void>::err("unmarshaling indicator");
        }
        const uint32_t ind = static_cast<uint32_t>(ind_item->valuedouble);
        if (ind == 0) {
            cJSON_Delete(array);
            return Result<void>::err("indicator field, if present, MUST be non-zero");
        }
        monad.indicator = Indicator(ind);
    } else {
        monad.indicator = Indicator(kIndicatorNone);
    }

    cJSON_Delete(array);
    monad.format = Format::JSONRecord;
    return Result<void>::ok();
}

Result<std::vector<uint8_t>> encode_record_json(const CMW::Monad& monad) {
    cJSON* array = cJSON_CreateArray();
    if (array == nullptr) {
        return Result<std::vector<uint8_t>>::err("failed to create JSON array");
    }

    if (monad.type.is_media_type()) {
        cJSON_AddItemToArray(array, cJSON_CreateString(monad.type.media_type().c_str()));
    } else if (monad.type.is_content_format()) {
        cJSON_AddItemToArray(array, cJSON_CreateNumber(monad.type.content_format()));
    } else {
        cJSON_Delete(array);
        return Result<std::vector<uint8_t>>::err("JSON record type must be a media type string");
    }

    const std::string encoded_value = base64url_encode(monad.value);
    cJSON_AddItemToArray(array, cJSON_CreateString(encoded_value.c_str()));

    if (!monad.indicator.empty()) {
        cJSON_AddItemToArray(array, cJSON_CreateNumber(monad.indicator.value()));
    }

    char* printed = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    if (printed == nullptr) {
        return Result<std::vector<uint8_t>>::err("failed to print JSON record");
    }

    std::vector<uint8_t> out(printed, printed + std::strlen(printed));
    cJSON_free(printed);
    return Result<std::vector<uint8_t>>::ok(std::move(out));
}

Result<std::vector<uint8_t>> encode_record_cbor(const CMW::Monad& monad) {
    UsefulBuf_MAKE_STACK_UB(buffer, kEncodeBufferSize);
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, buffer);
    QCBOREncode_OpenArray(&ctx);

    auto type_encoded = monad.type.encode_cbor();
    if (!type_encoded) {
        return Result<std::vector<uint8_t>>::err(type_encoded.error());
    }
    QCBOREncode_AddEncoded(&ctx, qcbor_util::to_useful_buf(type_encoded.value()));
    QCBOREncode_AddBytes(&ctx, qcbor_util::to_useful_buf(monad.value));

    if (!monad.indicator.empty()) {
        QCBOREncode_AddUInt64(&ctx, monad.indicator.value());
    }

    QCBOREncode_CloseArray(&ctx);
    return qcbor_util::finish_encode(ctx);
}

Result<std::vector<uint8_t>> encode_tag_cbor(const CMW::Monad& monad) {
    auto tn = monad.type.tag_number();
    if (!tn) {
        return Result<std::vector<uint8_t>>::err("getting a suitable tag value: " + tn.error());
    }

    UsefulBuf_MAKE_STACK_UB(buffer, kEncodeBufferSize);
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, buffer);
    QCBOREncode_AddTag(&ctx, tn.value());
    QCBOREncode_AddBytes(&ctx, qcbor_util::to_useful_buf(monad.value));
    return qcbor_util::finish_encode(ctx);
}

}  // namespace

Result<void> CMW::Monad::validate() const {
    if (!type.is_set()) {
        return Result<void>::err("type not set");
    }
    if (value.empty()) {
        return Result<void>::err("value not set");
    }
    if (!indicator.empty() && indicator.value() > kMaxIndicatorValue) {
        return Result<void>::err("indicator value " + std::to_string(indicator.value()) +
                                 " exceeds maximum " + std::to_string(kMaxIndicatorValue));
    }
    return Result<void>::ok();
}

Result<std::vector<uint8_t>> CMW::Monad::marshal_json() const {
    if (!type.is_set() || value.empty()) {
        return Result<std::vector<uint8_t>>::err("type and value MUST be set in CMW");
    }
    return encode_record_json(*this);
}

Result<std::vector<uint8_t>> CMW::Monad::marshal_cbor() const {
    if (!type.is_set() || value.empty()) {
        return Result<std::vector<uint8_t>>::err("type and value MUST be set in CMW");
    }
    if (format == Format::CBORTag) {
        return encode_tag_cbor(*this);
    }
    return encode_record_cbor(*this);
}

Result<void> CMW::Monad::unmarshal_json(const uint8_t* data, size_t len) {
    return decode_record_json(data, len, *this);
}

Result<void> CMW::Monad::unmarshal_cbor(const uint8_t* data, size_t len) {
    if (len == 0) {
        return Result<void>::err("empty buffer");
    }

    QCBORDecodeContext ctx;
    qcbor_util::init_decode(ctx, data, len);

    QCBORItem item;
    QCBORError err = QCBORDecode_GetNext(&ctx, &item);
    if (err != QCBOR_SUCCESS) {
        return Result<void>::err("decoding record");
    }

    if (item.uDataType == QCBOR_TYPE_ARRAY) {
        return decode_record_items(ctx, *this, item.val.uCount);
    }
    if (qcbor_util::item_has_tag(item)) {
        return decode_tag_item(ctx, item, *this);
    }
    return Result<void>::err("want CBOR Tag or CBOR array");
}

}  // namespace cmw
