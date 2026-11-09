#pragma once

#include "seventh/systems/entity_resolution/entity_types.hpp"

namespace seventh::entity_resolution {

class EntityClassifier {
public:
    [[nodiscard]] EntityType classify(const std::string& query) const;
};

} // namespace seventh::entity_resolution
