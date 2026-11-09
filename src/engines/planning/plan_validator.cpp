#include "seventh/engines/planning/plan_validator.hpp"

#include "seventh/engines/planning/dependency_analyzer.hpp"

#include <cctype>
#include <set>
#include <string>

namespace seventh::planning {
namespace {

bool is_blank(const std::string& value) {
    for (const unsigned char character : value) {
        if (!std::isspace(character)) return false;
    }
    return true;
}

} // namespace

StringList PlanValidator::validate(const Plan& plan) const {
    StringList errors;
    if (is_blank(plan.goal)) errors.emplace_back("The plan has no goal.");
    if (plan.tasks.empty()) errors.emplace_back("The plan contains no tasks.");

    if (plan.tasks.size() > plan.max_tasks) {
        errors.emplace_back("The plan exceeds its configured task limit.");
        return errors;
    }

    std::size_t dependency_count = 0;
    for (const auto& task : plan.tasks) {
        dependency_count += task.dependencies.size();
        if (dependency_count > plan.max_dependency_edges) {
            errors.emplace_back("The plan exceeds its configured dependency limit.");
            return errors;
        }
    }

    std::set<std::string> ids;
    for (const auto& task : plan.tasks) {
        if (is_blank(task.id)) errors.emplace_back("A task has no identifier.");
        if (!ids.insert(task.id).second) {
            errors.emplace_back("Duplicate task identifier: " + task.id);
        }
        if (is_blank(task.operation)) {
            errors.emplace_back("Task " + task.id + " has no operation.");
        }
        for (const auto& dependency : task.dependencies) {
            if (is_blank(dependency)) {
                errors.emplace_back("Task " + task.id + " has an empty dependency.");
            } else if (dependency == task.id) {
                errors.emplace_back("Task " + task.id + " cannot depend on itself.");
            }
        }
        for (const auto& [key, value] : task.input_data) {
            (void)value;
            if (is_blank(key)) {
                errors.emplace_back("Task " + task.id + " contains an empty input key.");
            }
        }
    }

    const auto dependencies = DependencyAnalyzer{}.analyze(plan);
    if (dependencies.has_duplicate_task_ids) {
        // Already reported above; this flag also prevents ambiguous graph analysis.
    }
    for (const auto& missing : dependencies.missing_dependencies) {
        errors.emplace_back("Missing dependency: " + missing);
    }
    if (dependencies.has_cycle) {
        errors.emplace_back("The task dependency graph contains a cycle.");
    }
    return errors;
}

} // namespace seventh::planning
