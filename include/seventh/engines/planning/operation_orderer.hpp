#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

class OperationOrderer {
public:
    [[nodiscard]] bool order(Plan& plan) const;
};

} // namespace seventh::planning
