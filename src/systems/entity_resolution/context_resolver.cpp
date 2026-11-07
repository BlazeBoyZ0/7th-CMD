#include "seventh/systems/entity_resolution/context_resolver.hpp"

namespace seventh::entity_resolution {

void ContextResolver::apply(
    const ResolutionRequest& request,
    std::vector<EntityCandidate>& candidates) const {
    for (auto& candidate : candidates) {
        if (request.expected_type != EntityType::Unknown &&
            candidate.type == request.expected_type) {
            candidate.score += 20;
        }
        if (!request.context_hint.empty() &&
            candidate.location.find(request.context_hint) != std::string::npos) {
            candidate.score += 10;
        }
    }
}

} // namespace seventh::entity_resolution
