#pragma once

#include "seventh/systems/entity_resolution/entity_types.hpp"

namespace seventh::entity_resolution {

class AmbiguityDetector {
public:
    [[nodiscard]] bool is_ambiguous(
        const std::vector<EntityCandidate>& ranked_candidates) const noexcept;
};

} // namespace seventh::entity_resolution
