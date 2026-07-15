#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef CMW_TESTDATA_DIR
#define CMW_TESTDATA_DIR "testdata"
#endif

inline std::string testdata_path(const std::string& name) {
    return std::string(CMW_TESTDATA_DIR) + "/" + name;
}

inline std::vector<uint8_t> read_file_bytes(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)),
                                  std::istreambuf_iterator<char>());
}

inline std::string read_file_text(const std::string& path) {
    std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

inline std::vector<uint8_t> hex_decode(const std::string& hex) {
    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex) {
        if (c == ' ' || c == '\n' || c == '\t') {
            continue;
        }
        cleaned.push_back(c);
    }

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    std::vector<uint8_t> out;
    out.reserve(cleaned.size() / 2);
    for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
        const int hi = nibble(cleaned[i]);
        const int lo = nibble(cleaned[i + 1]);
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}
