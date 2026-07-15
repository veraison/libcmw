#pragma once

#include <cstdint>

#include "cmw/result.hpp"

namespace cmw {

constexpr uint16_t kCfMin = 0;
constexpr uint16_t kCfMax = 65024;

constexpr uint64_t kTnMin = 1668546817ULL;
constexpr uint64_t kTnMax = 1668612095ULL;

Result<uint64_t> tn_from_content_format(uint16_t cf);
Result<uint16_t> content_format_from_tn(uint64_t tn);

}  // namespace cmw
