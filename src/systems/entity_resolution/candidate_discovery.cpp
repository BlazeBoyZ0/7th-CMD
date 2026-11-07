#include "seventh/systems/entity_resolution/candidate_discovery.hpp"

#include <set>
#include <utility>

namespace seventh::entity_resolution {

std::vector<EntityCandidate> CandidateDiscovery::discover(
    const ResolutionRequest& request) const {
    std::vector<EntityCandidate> result;
    std::set<std::pair<EntityType, std::string>> seen;

    for (const auto& candidate : request.candidates) {
        if (!candidate.available || candidate.id.empty() || candidate.name.empty()) {
            continue;
        }
        if (request.expected_type != EntityType::Unknown &&
            candidate.type != request.expected_type) {
            continue;
        }

        const auto identity = std::make_pair(candidate.type, candidate.id);
        if (seen.insert(identity).second) result.push_back(candidate);
    }

    return result;
}

} // namespace seventh::entity_resolution
