#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

class PlanOptimizer {
public:
    void optimize(Plan& plan) const;
};

} // namespace seventh::planning
