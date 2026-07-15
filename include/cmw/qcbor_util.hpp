#pragma once

#include <qcbor/qcbor.h>

#include <cstdint>
#include <string>
#include <vector>

#include "cmw/result.hpp"

namespace cmw::qcbor_util {

inline UsefulBufC to_useful_buf(const std::vector<uint8_t>& data) {
    return UsefulBufC{data.data(), data.size()};
}

inline UsefulBufC to_useful_buf(const uint8_t* data, size_t len) {
    return UsefulBufC{data, len};
}

inline UsefulBufC to_useful_buf(const std::string& text) {
    return UsefulBufC{text.data(), text.size()};
}

Result<std::vector<uint8_t>> finish_encode(QCBOREncodeContext& ctx);

void init_decode(QCBORDecodeContext& ctx, const uint8_t* data, size_t len);

bool is_uint_item(const QCBORItem& item);
uint64_t uint_item_value(const QCBORItem& item);

bool item_has_tag(const QCBORItem& item);
Result<uint64_t> item_tag_number(QCBORDecodeContext& ctx, const QCBORItem& item);

}  // namespace cmw::qcbor_util
