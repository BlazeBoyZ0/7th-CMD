#include "seventh/engines/planning/plan_validator.hpp"

#include "seventh/engines/planning/dependency_analyzer.hpp"

#include <set>

namespace seventh::planning {

StringList PlanValidator::validate(const Plan& plan) const {
    StringList errors;
    if (plan.goal.empty()) errors.emplace_back("The plan has no goal.");
    if (plan.tasks.empty()) errors.emplace_back("The plan contains no tasks.");

    std::set<std::string> ids;
    for (const auto& task : plan.tasks) {
        if (task.id.empty()) errors.emplace_back("A task has no identifier.");
        if (!ids.insert(task.id).second) errors.emplace_back("Duplicate task identifier: " + task.id);
        if (task.operation.empty()) errors.emplace_back("Task " + task.id + " has no operation.");
    }

    const auto dependencies = DependencyAnalyzer{}.analyze(plan);
    for (const auto& missing : dependencies.missing_dependencies) {
        errors.emplace_back("Missing dependency: " + missing);
    }
    if (dependencies.has_cycle) errors.emplace_back("The task dependency graph contains a cycle.");
    return errors;
}

} // namespace seventh::planning
