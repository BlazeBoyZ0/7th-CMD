#pragma once

#include "seventh/engines/planning/goal_decomposer.hpp"
#include "seventh/engines/planning/operation_orderer.hpp"
#include "seventh/engines/planning/plan_optimizer.hpp"
#include "seventh/engines/planning/plan_validator.hpp"
#include "seventh/engines/planning/task_breakdown.hpp"

namespace seventh::planning {

class PlanningEngine {
public:
    [[nodiscard]] Plan create_plan(const PlanningRequest& request) const;

private:
    TaskBreakdown task_breakdown_;
    GoalDecomposer goal_decomposer_;
    PlanValidator plan_validator_;
    OperationOrderer operation_orderer_;
    PlanOptimizer plan_optimizer_;
};

} // namespace seventh::planning
