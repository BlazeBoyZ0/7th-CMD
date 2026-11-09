#include "seventh/engines/planning/planning_engine.hpp"

namespace seventh::planning {

Plan PlanningEngine::create_plan(const PlanningRequest& request) const {
    auto normalized = request;
    normalized.tasks = task_breakdown_.normalize(request.tasks);
    auto plan = goal_decomposer_.decompose(normalized);

    plan.errors = plan_validator_.validate(plan);
    if (!plan.errors.empty()) return plan;

    if (!operation_orderer_.order(plan)) {
        plan.errors.emplace_back(
            "Unable to order tasks because dependencies are invalid or cyclic.");
        return plan;
    }

    plan_optimizer_.optimize(plan);

    // The optimizer is not allowed to weaken dependencies or leave the plan
    // in a different topological order. Validate the final artifact again.
    plan.errors = plan_validator_.validate(plan);
    if (!plan.errors.empty()) return plan;

    plan.valid = true;
    return plan;
}

} // namespace seventh::planning
