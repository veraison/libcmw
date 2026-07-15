#include "cmw/indicator.hpp"

#include <algorithm>
#include <array>
#include <sstream>

namespace cmw {

namespace {

constexpr std::array<std::pair<IndicatorFlag, const char*>, 5> kIndicatorNames = {{
    {IndicatorFlag::ReferenceValues, "reference values"},
    {IndicatorFlag::Endorsements, "endorsements"},
    {IndicatorFlag::Evidence, "evidence"},
    {IndicatorFlag::AttestationResults, "attestation results"},
    {IndicatorFlag::AppraisalPolicy, "appraisal policy"},
}};

}  // namespace

std::string Indicator::to_string() const {
    std::vector<std::string> names;
    for (const auto& entry : kIndicatorNames) {
        if (has(entry.first)) {
            names.emplace_back(entry.second);
        }
    }
    std::sort(names.begin(), names.end());
    std::ostringstream oss;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << names[i];
    }
    return oss.str();
}

}  // namespace cmw
