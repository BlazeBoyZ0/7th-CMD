#include "seventh/systems/entity_resolution/context_resolver.hpp"

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

void ContextResolver::apply(
    const ResolutionRequest& request,
    std::vector<EntityCandidate>& candidates) const {
    const auto context_hint = lower(request.context_hint);

    for (auto& candidate : candidates) {
        // Candidate discovery already enforces the requested entity type.
        // Context must refine a name match, never manufacture one.
        if (candidate.score > 0 && !context_hint.empty() &&
            lower(candidate.location).find(context_hint) != std::string::npos) {
            candidate.score += 10;
        }
    }
}

} // namespace seventh::entity_resolution
