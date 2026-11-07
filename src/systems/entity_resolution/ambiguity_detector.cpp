#include "seventh/systems/entity_resolution/ambiguity_detector.hpp"

namespace seventh::entity_resolution {

bool AmbiguityDetector::is_ambiguous(
    const std::vector<EntityCandidate>& candidates) const noexcept {
    return candidates.size() > 1 &&
           candidates[0].score > 0 &&
           candidates[1].score > 0 &&
           candidates[0].score - candidates[1].score < 10;
}

} // namespace seventh::entity_resolution
