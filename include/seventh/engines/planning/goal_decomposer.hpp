#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

class GoalDecomposer {
public:
    [[nodiscard]] Plan decompose(const PlanningRequest& request) const;
};

} // namespace seventh::planning
