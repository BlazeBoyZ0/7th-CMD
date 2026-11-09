#include "seventh/engines/planning/planning_engine.hpp"

#include "seventh/core/identifiers.hpp"

namespace seventh::planning {

Plan PlanningEngine::create_plan(const PlanningRequest& request) const {
    // Enforce input-size budgets before normalization or decomposition copies
    // caller-owned task/dependency collections.
    Plan rejected;
    rejected.id = make_identifier("plan");
    rejected.goal = request.goal;
    rejected.max_tasks = request.max_tasks;
    rejected.max_dependency_edges = request.max_dependency_edges;

    if (request.tasks.size() > request.max_tasks) {
        rejected.errors.emplace_back(
            "The plan exceeds its configured task limit.");
        return rejected;
    }

    std::size_t dependency_count = 0;
    for (const auto& task : request.tasks) {
        if (task.dependencies.size() >
            request.max_dependency_edges - dependency_count) {
            rejected.errors.emplace_back(
                "The plan exceeds its configured dependency limit.");
            return rejected;
        }
        dependency_count += task.dependencies.size();
    }

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
