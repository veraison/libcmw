#include "cmw/util.hpp"

#include <cctype>

namespace cmw {

namespace {

constexpr char kBase64UrlAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int base64url_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

}  // namespace

std::string base64url_encode(const std::vector<uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i < data.size()) {
        const uint32_t octet_a = data[i++];
        const uint32_t octet_b = i < data.size() ? data[i++] : 0;
        const uint32_t octet_c = i < data.size() ? data[i++] : 0;
        const uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        out.push_back(kBase64UrlAlphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64UrlAlphabet[(triple >> 12) & 0x3F]);
        if (i > data.size() + 1) {
            out.push_back(kBase64UrlAlphabet[(triple >> 6) & 0x3F]);
        } else if (i > data.size()) {
            out.push_back(kBase64UrlAlphabet[(triple >> 6) & 0x3F]);
        } else {
            out.push_back(kBase64UrlAlphabet[(triple >> 6) & 0x3F]);
            out.push_back(kBase64UrlAlphabet[triple & 0x3F]);
        }
    }

    const size_t mod = data.size() % 3;
    if (mod == 1) {
        out.resize(out.size() - 2);
    } else if (mod == 2) {
        out.resize(out.size() - 1);
    }
    return out;
}

Result<std::vector<uint8_t>> base64url_decode(const std::string& data) {
    if (data.find('=') != std::string::npos) {
        return Result<std::vector<uint8_t>>::err("illegal base64 data at input byte " +
                                                 std::to_string(data.find('=')));
    }

    std::vector<uint8_t> out;
    out.reserve((data.size() * 3) / 4);

    uint32_t buffer = 0;
    int bits = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        const int value = base64url_value(data[i]);
        if (value < 0) {
            return Result<std::vector<uint8_t>>::err("illegal base64 data at input byte " +
                                                     std::to_string(i));
        }
        buffer = (buffer << 6) | static_cast<uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((buffer >> bits) & 0xFF));
        }
    }
    return Result<std::vector<uint8_t>>::ok(std::move(out));
}

bool is_valid_media_type(const std::string& media_type) {
    if (media_type.empty()) {
        return false;
    }

    const auto slash = media_type.find('/');
    if (slash == std::string::npos || slash == 0) {
        return false;
    }

    const auto type = media_type.substr(0, slash);
    for (char c : type) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc) && c != '.' && c != '+' && c != '-') {
            return false;
        }
    }

    if (slash + 1 >= media_type.size()) {
        return false;
    }

    const auto subtype_start = slash + 1;
    if (media_type[subtype_start] == ' ') {
        return false;
    }

    return true;
}

bool start_json_collection(uint8_t byte) { return byte == 0x7b; }
bool start_json_record(uint8_t byte) { return byte == 0x5b; }

bool start_cbor_collection(uint8_t byte) {
    return (byte >= 0xa0 && byte <= 0xbb) || byte == 0xbf;
}

bool start_cbor_record(uint8_t byte) {
    return byte == 0x82 || byte == 0x83 || byte == 0x9f;
}

bool start_cbor_tag(uint8_t byte) { return byte >= 0xda; }

std::string collection_key_sort_string(const CollectionKey& key) {
    if (std::holds_alternative<std::string>(key)) {
        return std::get<std::string>(key);
    }
    return "##" + std::to_string(std::get<uint64_t>(key));
}

}  // namespace cmw
