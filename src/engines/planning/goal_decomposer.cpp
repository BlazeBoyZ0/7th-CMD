#include "seventh/engines/planning/goal_decomposer.hpp"

#include "seventh/core/identifiers.hpp"

namespace seventh::planning {

Plan GoalDecomposer::decompose(const PlanningRequest& request) const {
    Plan plan;
    plan.id = make_identifier("plan");
    plan.goal = request.goal;
    plan.tasks.reserve(request.tasks.size());

    for (const auto& draft : request.tasks) {
        PlanTask task;
        task.id = make_identifier("step");
        task.operation = draft.operation;
        task.input_data = draft.input_data;
        task.dependencies = draft.dependencies;
        plan.tasks.push_back(std::move(task));
    }
    return plan;
}

} // namespace seventh::planning
