#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cmw {

enum class IndicatorFlag : uint32_t {
    ReferenceValues = 1u << 0,
    Endorsements = 1u << 1,
    Evidence = 1u << 2,
    AttestationResults = 1u << 3,
    AppraisalPolicy = 1u << 4,
};

constexpr uint32_t kIndicatorNone = 0;
constexpr uint32_t kMaxIndicatorValue = (static_cast<uint32_t>(IndicatorFlag::AppraisalPolicy) << 1) - 1;

class Indicator {
public:
    Indicator() = default;
    explicit Indicator(uint32_t value) : value_(value) {}

    void set(IndicatorFlag flag) { value_ |= static_cast<uint32_t>(flag); }
    void clear(IndicatorFlag flag) { value_ &= ~static_cast<uint32_t>(flag); }
    void toggle(IndicatorFlag flag) { value_ ^= static_cast<uint32_t>(flag); }
    bool has(IndicatorFlag flag) const {
        return (value_ & static_cast<uint32_t>(flag)) != 0;
    }
    bool empty() const { return value_ == kIndicatorNone; }
    uint32_t value() const { return value_; }

    std::string to_string() const;

private:
    uint32_t value_ = kIndicatorNone;
};

}  // namespace cmw
