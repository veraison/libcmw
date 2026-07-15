#include "cmw/type.hpp"

#include <cJSON.h>
#include <qcbor/qcbor.h>

#include "cmw/cfmap.hpp"
#include "cmw/qcbor_util.hpp"
#include "cmw/tn.hpp"
#include "cmw/util.hpp"

namespace cmw {

Result<void> Type::set_media_type(const std::string& media_type) {
    if (!is_valid_media_type(media_type)) {
        return Result<void>::err("bad media type: invalid media type syntax");
    }
    value_ = media_type;
    return Result<void>::ok();
}

Result<void> Type::set_content_format(uint16_t cf) {
    value_ = cf;
    return Result<void>::ok();
}

Result<void> Type::set_tag_number(uint64_t tn) {
    value_ = tn;
    return Result<void>::ok();
}

bool Type::is_set() const {
    if (std::holds_alternative<std::monostate>(value_)) {
        return false;
    }
    if (std::holds_alternative<std::string>(value_)) {
        return !std::get<std::string>(value_).empty();
    }
    return true;
}

std::string Type::to_string() const {
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    }
    if (std::holds_alternative<uint16_t>(value_)) {
        const uint16_t cf = std::get<uint16_t>(value_);
        if (auto mt = content_format_to_media_type(cf)) {
            return *mt;
        }
        return std::to_string(cf);
    }
    if (std::holds_alternative<uint64_t>(value_)) {
        const uint64_t tn = std::get<uint64_t>(value_);
        auto cf = content_format_from_tn(tn);
        if (!cf) {
            return "";
        }
        if (auto mt = content_format_to_media_type(cf.value())) {
            return *mt;
        }
        return std::to_string(cf.value());
    }
    return "";
}

Result<uint64_t> Type::tag_number() const {
    if (std::holds_alternative<std::string>(value_)) {
        auto cf = media_type_to_content_format(std::get<std::string>(value_));
        if (!cf) {
            return Result<uint64_t>::err(
                "media type \"" + std::get<std::string>(value_) +
                "\" has no registered CoAP Content-Format");
        }
        return tn_from_content_format(*cf);
    }
    if (std::holds_alternative<uint16_t>(value_)) {
        return tn_from_content_format(std::get<uint16_t>(value_));
    }
    if (std::holds_alternative<uint64_t>(value_)) {
        return Result<uint64_t>::ok(std::get<uint64_t>(value_));
    }
    return Result<uint64_t>::err("type not set");
}

Result<std::vector<uint8_t>> Type::encode_json() const {
    if (!is_set()) {
        return Result<std::vector<uint8_t>>::err("type not set");
    }
    if (!std::holds_alternative<std::string>(value_)) {
        return Result<std::vector<uint8_t>>::err("JSON record type must be a media type string");
    }
    const auto& mt = std::get<std::string>(value_);
    std::string json = "\"" + mt + "\"";
    return Result<std::vector<uint8_t>>::ok(std::vector<uint8_t>(json.begin(), json.end()));
}

Result<std::vector<uint8_t>> Type::encode_cbor() const {
    if (!is_set()) {
        return Result<std::vector<uint8_t>>::err("type not set");
    }

    UsefulBuf_MAKE_STACK_UB(buffer, 256);
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, buffer);

    if (std::holds_alternative<std::string>(value_)) {
        QCBOREncode_AddSZString(&ctx, std::get<std::string>(value_).c_str());
    } else if (std::holds_alternative<uint16_t>(value_)) {
        QCBOREncode_AddUInt64(&ctx, std::get<uint16_t>(value_));
    } else {
        return Result<std::vector<uint8_t>>::err("wrong type for Type");
    }

    return qcbor_util::finish_encode(ctx);
}

Result<void> Type::decode_json(const uint8_t* data, size_t len) {
    const std::string raw(reinterpret_cast<const char*>(data), len);
    cJSON* item = cJSON_Parse(raw.c_str());
    if (item == nullptr) {
        return Result<void>::err("cannot unmarshal JSON type");
    }

    Result<void> result = Result<void>::ok();
    if (cJSON_IsString(item)) {
        result = set_media_type(item->valuestring);
    } else if (cJSON_IsNumber(item)) {
        const double number = item->valuedouble;
        if (number != static_cast<double>(static_cast<uint16_t>(number))) {
            result = Result<void>::err("cannot unmarshal " + std::to_string(number) + " into uint16");
        } else if (number > UINT16_MAX) {
            result = Result<void>::err("cannot unmarshal " + std::to_string(number) + " into uint16");
        } else {
            result = set_content_format(static_cast<uint16_t>(number));
        }
    } else {
        result = Result<void>::err("expecting string or uint16");
    }

    cJSON_Delete(item);
    return result;
}

Result<void> Type::decode_cbor(const uint8_t* data, size_t len) {
    QCBORDecodeContext ctx;
    qcbor_util::init_decode(ctx, data, len);

    QCBORItem item;
    QCBORError err = QCBORDecode_GetNext(&ctx, &item);
    if (err != QCBOR_SUCCESS) {
        return Result<void>::err("cannot unmarshal CBOR type");
    }

    if (item.uDataType == QCBOR_TYPE_TEXT_STRING) {
        std::string mt(reinterpret_cast<const char*>(item.val.string.ptr), item.val.string.len);
        return set_media_type(mt);
    }

    if (qcbor_util::is_uint_item(item)) {
        const uint64_t value = qcbor_util::uint_item_value(item);
        if (value > UINT16_MAX) {
            return Result<void>::err("cannot unmarshal " + std::to_string(value) + " into uint16");
        }
        return set_content_format(static_cast<uint16_t>(value));
    }

    return Result<void>::err("expecting string or uint16");
}

}  // namespace cmw
