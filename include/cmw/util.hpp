#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cmw/cmw.hpp"
#include "cmw/result.hpp"

namespace cmw {

std::string base64url_encode(const std::vector<uint8_t>& data);
Result<std::vector<uint8_t>> base64url_decode(const std::string& data);

bool is_valid_media_type(const std::string& media_type);

bool start_json_collection(uint8_t byte);
bool start_json_record(uint8_t byte);
bool start_cbor_collection(uint8_t byte);
bool start_cbor_record(uint8_t byte);
bool start_cbor_tag(uint8_t byte);

std::string collection_key_sort_string(const CollectionKey& key);

}  // namespace cmw
