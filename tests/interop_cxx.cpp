#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cmw/cmw.hpp"

namespace {

std::vector<uint8_t> hex_decode(const std::string& hex) {
    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex) {
        if (c == ' ' || c == '\n' || c == '\t') {
            continue;
        }
        cleaned.push_back(c);
    }

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    };

    std::vector<uint8_t> out;
    out.reserve(cleaned.size() / 2);
    for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
        const int hi = nibble(cleaned[i]);
        const int lo = nibble(cleaned[i + 1]);
        if (hi < 0 || lo < 0) {
            throw std::runtime_error("invalid hex input");
        }
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}

std::string hex_encode(const std::vector<uint8_t>& data) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (uint8_t byte : data) {
        out.push_back(digits[byte >> 4]);
        out.push_back(digits[byte & 0x0f]);
    }
    return out;
}

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        fail("usage: cmw_interop <roundtrip-cbor|roundtrip-json|decode-kind> <payload>");
    }

    const std::string mode = argv[1];
    const std::string payload = argv[2];

    try {
        if (mode == "roundtrip-cbor") {
            const auto input = hex_decode(payload);
            auto decoded = cmw::decode(input);
            if (!decoded) {
                fail(decoded.error());
            }
            if (!decoded.value().valid()) {
                fail("validation failed");
            }
            auto output = decoded.value().marshal_cbor();
            if (!output) {
                fail(output.error());
            }
            std::cout << hex_encode(output.value());
            return 0;
        }

        if (mode == "roundtrip-json") {
            const std::vector<uint8_t> input(payload.begin(), payload.end());
            auto decoded = cmw::decode(input);
            if (!decoded) {
                fail(decoded.error());
            }
            if (!decoded.value().valid()) {
                fail("validation failed");
            }
            auto output = decoded.value().marshal_json();
            if (!output) {
                fail(output.error());
            }
            std::cout.write(reinterpret_cast<const char*>(output.value().data()),
                            static_cast<std::streamsize>(output.value().size()));
            return 0;
        }

        if (mode == "decode-kind") {
            const auto input = hex_decode(payload);
            auto decoded = cmw::decode(input);
            if (!decoded) {
                fail(decoded.error());
            }
            std::cout << cmw::kind_to_string(decoded.value().kind());
            return 0;
        }

        fail("unknown mode: " + mode);
    } catch (const std::exception& ex) {
        fail(ex.what());
    }
}
