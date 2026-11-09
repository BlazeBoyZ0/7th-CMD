#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

class PlanValidator {
public:
    [[nodiscard]] StringList validate(const Plan& plan) const;
};

} // namespace seventh::planning
