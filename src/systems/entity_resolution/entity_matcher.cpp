#include "seventh/systems/entity_resolution/entity_matcher.hpp"

#include <algorithm>
#include <cctype>

namespace seventh::entity_resolution {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

} // namespace

int EntityMatcher::score(const std::string& query,
                         const EntityCandidate& candidate) const {
    if (query.empty() || candidate.name.empty()) return 0;
    const auto wanted = lower(query);
    const auto actual = lower(candidate.name);
    if (wanted == actual) return 100;
    if (actual.find(wanted) != std::string::npos) return 75;
    if (wanted.find(actual) != std::string::npos) return 65;
    return 0;
}

} // namespace seventh::entity_resolution
