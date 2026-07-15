#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "cmw/result.hpp"

namespace cmw {

class Type {
public:
    Type() = default;

    Result<void> set_media_type(const std::string& media_type);
    Result<void> set_content_format(uint16_t cf);
    Result<void> set_tag_number(uint64_t tn);

    bool is_set() const;
    std::string to_string() const;
    Result<uint64_t> tag_number() const;

    bool is_media_type() const { return std::holds_alternative<std::string>(value_); }
    bool is_content_format() const { return std::holds_alternative<uint16_t>(value_); }
    bool is_tag_number() const { return std::holds_alternative<uint64_t>(value_); }

    const std::string& media_type() const { return std::get<std::string>(value_); }
    uint16_t content_format() const { return std::get<uint16_t>(value_); }
    uint64_t tag_number_value() const { return std::get<uint64_t>(value_); }

    Result<std::vector<uint8_t>> encode_json() const;
    Result<std::vector<uint8_t>> encode_cbor() const;
    Result<void> decode_json(const uint8_t* data, size_t len);
    Result<void> decode_cbor(const uint8_t* data, size_t len);

    bool operator==(const Type& other) const { return value_ == other.value_; }

private:
    std::variant<std::monostate, std::string, uint16_t, uint64_t> value_;
};

}  // namespace cmw
