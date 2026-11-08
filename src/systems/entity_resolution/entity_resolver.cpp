#include "seventh/systems/entity_resolution/entity_resolver.hpp"

#include <algorithm>

namespace seventh::entity_resolution {

ResolutionResult EntityResolver::resolve(ResolutionRequest request) const {
    ResolutionResult result;
    if (request.query.empty()) {
        result.explanation = "No entity name was provided.";
        return result;
    }

    if (request.expected_type == EntityType::Unknown) {
        request.expected_type = classifier_.classify(request.query);
    }

    result.candidates = discovery_.discover(request);
    for (auto& candidate : result.candidates) {
        candidate.score = matcher_.score(request.query, candidate);
    }
    context_resolver_.apply(request, result.candidates);
    result.candidates.erase(
        std::remove_if(result.candidates.begin(), result.candidates.end(),
            [](const EntityCandidate& candidate) { return candidate.score <= 0; }),
        result.candidates.end());

    std::stable_sort(result.candidates.begin(), result.candidates.end(),
        [](const EntityCandidate& left, const EntityCandidate& right) {
            return left.score > right.score;
        });

    if (result.candidates.empty() || result.candidates.front().score == 0) {
        result.explanation = "No available candidate matches the requested entity.";
        return result;
    }

    if (ambiguity_detector_.is_ambiguous(result.candidates)) {
        result.status = ResolutionStatus::Ambiguous;
        result.explanation = "Multiple candidates have similarly strong matches.";
        return result;
    }

    result.status = ResolutionStatus::Found;
    result.selected = result.candidates.front();
    result.explanation = "A unique best candidate was found.";
    return result;
}

} // namespace seventh::entity_resolution
