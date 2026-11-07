#include "seventh/systems/entity_resolution/candidate_discovery.hpp"

namespace seventh::entity_resolution {

std::vector<EntityCandidate> CandidateDiscovery::discover(
    const ResolutionRequest& request) const {
    std::vector<EntityCandidate> result;
    for (const auto& candidate : request.candidates) {
        if (candidate.available) result.push_back(candidate);
    }
    return result;
}

} // namespace seventh::entity_resolution
