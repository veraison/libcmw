#include "cmw/cmw.hpp"

#include <cJSON.h>
#include <qcbor/qcbor.h>

#include <algorithm>
#include <cstring>
#include <regex>
#include <sstream>

#include "cmw/cbor_internal.hpp"
#include "cmw/qcbor_util.hpp"
#include "cmw/util.hpp"

namespace cmw {

namespace {

constexpr char kCmwcTypeKey[] = "__cmwc_t";
constexpr size_t kEncodeBufferSize = 8192;

const std::regex kOidRegex(R"(^([0-2])(([.]0)|([.][1-9][0-9]*))*$)");

Result<std::vector<uint8_t>> encode_cbor_key(const CollectionKey& key) {
    UsefulBuf_MAKE_STACK_UB(buffer, 128);
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, buffer);

    if (std::holds_alternative<std::string>(key)) {
        QCBOREncode_AddSZString(&ctx, std::get<std::string>(key).c_str());
    } else {
        QCBOREncode_AddUInt64(&ctx, std::get<uint64_t>(key));
    }

    return qcbor_util::finish_encode(ctx);
}

}  // namespace

bool validate_collection_type(const std::string& cmwc_t) {
    if (cmwc_t.empty()) {
        return false;
    }
    if (std::regex_match(cmwc_t, kOidRegex)) {
        return true;
    }

    if (cmwc_t[0] == '/' || cmwc_t[0] == '.') {
        return false;
    }

    const auto colon = cmwc_t.find(':');
    if (colon == std::string::npos) {
        return false;
    }

    const std::string scheme = cmwc_t.substr(0, colon);
    if (scheme.empty()) {
        return false;
    }

    if (scheme == "tag" || scheme == "urn") {
        return colon + 1 < cmwc_t.size();
    }

    return cmwc_t.find("://", colon) == colon;
}

Result<void> validate_collection_key(const CollectionKey& key) {
    if (std::holds_alternative<std::string>(key)) {
        const auto& label = std::get<std::string>(key);
        if (label == kCmwcTypeKey) {
            return Result<void>::err(std::string("bad collection key: ") + kCmwcTypeKey +
                                     " is reserved");
        }
        if (label.find_first_not_of(" \t\n\r") == std::string::npos) {
            return Result<void>::err("bad collection key: empty or whitespace only");
        }
        return Result<void>::ok();
    }
    if (std::holds_alternative<uint64_t>(key)) {
        return Result<void>::ok();
    }
    return Result<void>::err("unknown collection key type");
}

Result<void> CMW::Collection::validate() const {
    if (items.empty()) {
        return Result<void>::err("empty CMW collection");
    }

    for (const auto& [key, value] : items) {
        auto valid = value.valid();
        if (!valid) {
            std::ostringstream oss;
            oss << "invalid ";
            if (value.kind() == Kind::Collection) {
                oss << "collection";
            } else {
                oss << "monad";
            }
            oss << " at key ";
            if (std::holds_alternative<std::string>(key)) {
                oss << '"' << std::get<std::string>(key) << '"';
            } else {
                oss << std::get<uint64_t>(key);
            }
            oss << ": " << valid.error();
            return Result<void>::err(oss.str());
        }
    }
    return Result<void>::ok();
}

Result<std::vector<uint8_t>> CMW::Collection::marshal_json() const {
    cJSON* object = cJSON_CreateObject();
    if (object == nullptr) {
        return Result<std::vector<uint8_t>>::err("failed to create JSON collection");
    }

    std::vector<std::pair<std::string, std::string>> entries;
    if (!cmwc_t.empty()) {
        entries.emplace_back(kCmwcTypeKey, "\"" + cmwc_t + "\"");
    }

    for (const auto& [key, value] : items) {
        if (!std::holds_alternative<std::string>(key)) {
            cJSON_Delete(object);
            return Result<std::vector<uint8_t>>::err("JSON collection, key error: want string");
        }
        auto encoded = value.marshal_json();
        if (!encoded) {
            cJSON_Delete(object);
            return Result<std::vector<uint8_t>>::err("marshaling JSON collection item: " +
                                                    encoded.error());
        }
        entries.emplace_back(std::get<std::string>(key),
                             std::string(encoded.value().begin(), encoded.value().end()));
    }

    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [key, value_json] : entries) {
        cJSON* value = cJSON_Parse(value_json.c_str());
        if (value == nullptr) {
            cJSON_Delete(object);
            return Result<std::vector<uint8_t>>::err("marshaling JSON collection");
        }
        cJSON_AddItemToObject(object, key.c_str(), value);
    }

    char* printed = cJSON_PrintUnformatted(object);
    cJSON_Delete(object);
    if (printed == nullptr) {
        return Result<std::vector<uint8_t>>::err("marshaling JSON collection");
    }

    std::vector<uint8_t> out(printed, printed + std::strlen(printed));
    cJSON_free(printed);
    return Result<std::vector<uint8_t>>::ok(std::move(out));
}

Result<std::vector<uint8_t>> CMW::Collection::marshal_cbor() const {
    struct Entry {
        std::vector<uint8_t> key_bytes;
        std::vector<uint8_t> value_bytes;
    };

    std::vector<Entry> entries;
    if (!cmwc_t.empty()) {
        auto key = encode_cbor_key(CollectionKey{std::string{kCmwcTypeKey}});
        if (!key) {
            return Result<std::vector<uint8_t>>::err(key.error());
        }

        UsefulBuf_MAKE_STACK_UB(buffer, 256);
        QCBOREncodeContext ctx;
        QCBOREncode_Init(&ctx, buffer);
        QCBOREncode_AddSZString(&ctx, cmwc_t.c_str());
        auto encoded = qcbor_util::finish_encode(ctx);
        if (!encoded) {
            return Result<std::vector<uint8_t>>::err(encoded.error());
        }
        entries.push_back({std::move(key.value()), std::move(encoded.value())});
    }

    for (const auto& [key, value] : items) {
        auto key_bytes = encode_cbor_key(key);
        if (!key_bytes) {
            return Result<std::vector<uint8_t>>::err(key_bytes.error());
        }
        auto value_bytes = value.marshal_cbor();
        if (!value_bytes) {
            return Result<std::vector<uint8_t>>::err("marshaling CBOR collection item: " +
                                                    value_bytes.error());
        }
        entries.push_back({std::move(key_bytes.value()), std::move(value_bytes.value())});
    }

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        const size_t min_len = std::min(a.key_bytes.size(), b.key_bytes.size());
        for (size_t i = 0; i < min_len; ++i) {
            if (a.key_bytes[i] != b.key_bytes[i]) {
                return a.key_bytes[i] < b.key_bytes[i];
            }
        }
        return a.key_bytes.size() < b.key_bytes.size();
    });

    UsefulBuf_MAKE_STACK_UB(buffer, kEncodeBufferSize);
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, buffer);
    QCBOREncode_OpenMap(&ctx);

    for (const auto& entry : entries) {
        QCBOREncode_AddEncoded(&ctx, qcbor_util::to_useful_buf(entry.key_bytes));
        QCBOREncode_AddEncoded(&ctx, qcbor_util::to_useful_buf(entry.value_bytes));
    }

    QCBOREncode_CloseMap(&ctx);
    return qcbor_util::finish_encode(ctx);
}

Result<void> CMW::Collection::unmarshal_cbor(const uint8_t* data, size_t len) {
    QCBORDecodeContext ctx;
    qcbor_util::init_decode(ctx, data, len);

    QCBORItem map_item;
    QCBORError err = QCBORDecode_GetNext(&ctx, &map_item);
    if (err != QCBOR_SUCCESS || map_item.uDataType != QCBOR_TYPE_MAP) {
        return Result<void>::err("unmarshaling CBOR collection");
    }

    CMW wrapper;
    auto result = cbor_internal::decode_collection_from_context(ctx, map_item, wrapper);
    if (!result) {
        return result;
    }

    items = std::move(wrapper.collection_.items);
    cmwc_t = std::move(wrapper.collection_.cmwc_t);
    format = Format::CBORCollection;
    return Result<void>::ok();
}

Result<void> CMW::Collection::unmarshal_json(const uint8_t* data, size_t len) {
    const std::string raw(reinterpret_cast<const char*>(data), len);
    cJSON* object = cJSON_Parse(raw.c_str());
    if (object == nullptr) {
        return Result<void>::err("unmarshaling JSON collection");
    }
    if (!cJSON_IsObject(object)) {
        cJSON_Delete(object);
        return Result<void>::err("unmarshaling JSON collection");
    }

    items.clear();
    cmwc_t.clear();

    cJSON* child = object->child;
    while (child != nullptr) {
        const std::string key = child->string ? child->string : "";
        if (key == kCmwcTypeKey) {
            if (!cJSON_IsString(child)) {
                cJSON_Delete(object);
                return Result<void>::err("extracting JSON collection type");
            }
            cmwc_t = child->valuestring;
            if (!validate_collection_type(cmwc_t)) {
                cJSON_Delete(object);
                return Result<void>::err("checking JSON collection type: invalid collection type");
            }
        } else {
            char* value_json = cJSON_PrintUnformatted(child);
            if (value_json == nullptr) {
                cJSON_Delete(object);
                return Result<void>::err("unmarshaling JSON collection");
            }

            CMW nested;
            auto nested_result =
                nested.unmarshal_json(reinterpret_cast<const uint8_t*>(value_json),
                                      std::strlen(value_json));
            cJSON_free(value_json);
            if (!nested_result) {
                cJSON_Delete(object);
                return Result<void>::err("unmarshaling JSON collection item " + key + ": " +
                                         nested_result.error());
            }
            items.emplace(key, std::move(nested));
        }
        child = child->next;
    }

    cJSON_Delete(object);
    format = Format::JSONCollection;
    return Result<void>::ok();
}

}  // namespace cmw
