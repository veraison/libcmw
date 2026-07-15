#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace cmw {

std::optional<uint16_t> media_type_to_content_format(const std::string& media_type);
std::optional<std::string> content_format_to_media_type(uint16_t cf);

}  // namespace cmw
