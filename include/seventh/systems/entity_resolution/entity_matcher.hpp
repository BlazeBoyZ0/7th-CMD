#pragma once

#include "seventh/systems/entity_resolution/entity_types.hpp"

namespace seventh::entity_resolution {

class EntityMatcher {
public:
    [[nodiscard]] int score(const std::string& query,
                            const EntityCandidate& candidate) const;
};

} // namespace seventh::entity_resolution
