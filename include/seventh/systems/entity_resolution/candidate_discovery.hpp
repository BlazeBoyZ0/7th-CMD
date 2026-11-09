#pragma once

#include "seventh/systems/entity_resolution/entity_types.hpp"

namespace seventh::entity_resolution {

class CandidateDiscovery {
public:
    [[nodiscard]] std::vector<EntityCandidate> discover(
        const ResolutionRequest& request) const;
};

} // namespace seventh::entity_resolution
