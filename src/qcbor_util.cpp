#include "cmw/qcbor_util.hpp"

#include <string>

namespace cmw::qcbor_util {

Result<std::vector<uint8_t>> finish_encode(QCBOREncodeContext& ctx) {
    UsefulBufC encoded;
    const QCBORError err = QCBOREncode_Finish(&ctx, &encoded);
    if (err != QCBOR_SUCCESS) {
        return Result<std::vector<uint8_t>>::err("QCBOR encode failed: " + std::to_string(err));
    }
    const auto* bytes = static_cast<const uint8_t*>(encoded.ptr);
    return Result<std::vector<uint8_t>>::ok(std::vector<uint8_t>(bytes, bytes + encoded.len));
}

void init_decode(QCBORDecodeContext& ctx, const uint8_t* data, size_t len) {
    QCBORDecode_Init(&ctx, UsefulBufC{data, len}, QCBOR_DECODE_MODE_NORMAL);
}

bool is_uint_item(const QCBORItem& item) {
    return item.uDataType == QCBOR_TYPE_UINT64 ||
           (item.uDataType == QCBOR_TYPE_INT64 && item.val.int64 >= 0);
}

uint64_t uint_item_value(const QCBORItem& item) {
    if (item.uDataType == QCBOR_TYPE_UINT64) {
        return item.val.uint64;
    }
    return static_cast<uint64_t>(item.val.int64);
}

bool item_has_tag(const QCBORItem& item) {
#ifndef QCBOR_DISABLE_TAGS
    return item.uTags[0] != CBOR_TAG_INVALID16;
#else
    (void)item;
    return false;
#endif
}

Result<uint64_t> item_tag_number(QCBORDecodeContext& ctx, const QCBORItem& item) {
#ifndef QCBOR_DISABLE_TAGS
    const uint64_t tag = QCBORDecode_GetNthTag(&ctx, &item, 0);
    if (tag == CBOR_TAG_INVALID64) {
        return Result<uint64_t>::err("invalid tag");
    }
    return Result<uint64_t>::ok(tag);
#else
    (void)ctx;
    (void)item;
    return Result<uint64_t>::err("tags disabled");
#endif
}

}  // namespace cmw::qcbor_util
