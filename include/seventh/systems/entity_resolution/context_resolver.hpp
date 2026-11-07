#pragma once

#include "seventh/systems/entity_resolution/entity_types.hpp"

namespace seventh::entity_resolution {

class ContextResolver {
public:
    void apply(ResolutionRequest const& request,
               std::vector<EntityCandidate>& candidates) const;
};

} // namespace seventh::entity_resolution
