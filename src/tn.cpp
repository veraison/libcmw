#include "cmw/tn.hpp"

#include <sstream>

namespace cmw {

Result<uint64_t> tn_from_content_format(uint16_t cf) {
    if (cf > kCfMax) {
        return Result<uint64_t>::err("C-F ID " + std::to_string(cf) + " out of range");
    }
    const uint64_t cf64 = cf;
    const uint64_t tn = kTnMin + (cf64 / 255) * 256 + cf64 % 255;
    return Result<uint64_t>::ok(tn);
}

Result<uint16_t> content_format_from_tn(uint64_t tn) {
    if (tn < kTnMin || tn > kTnMax) {
        return Result<uint16_t>::err("TN " + std::to_string(tn) + " out of range");
    }
    const uint64_t offset = tn - kTnMin;
    const uint16_t cf = static_cast<uint16_t>(offset * (256 / 255) - offset / 256);
    return Result<uint16_t>::ok(cf);
}

}  // namespace cmw
