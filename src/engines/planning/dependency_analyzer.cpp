#include "seventh/engines/planning/dependency_analyzer.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

namespace seventh::planning {

DependencyAnalysis DependencyAnalyzer::analyze(const Plan& plan) const {
    DependencyAnalysis result;
    std::set<std::string> ids;
    for (const auto& task : plan.tasks) ids.insert(task.id);

    std::map<std::string, int> marks;
    for (const auto& task : plan.tasks) {
        for (const auto& dependency : task.dependencies) {
            if (!ids.contains(dependency)) result.missing_dependencies.push_back(dependency);
        }
    }

    std::function<bool(const std::string&)> visit = [&](const std::string& id) {
        if (marks[id] == 1) return true;
        if (marks[id] == 2) return false;
        marks[id] = 1;
        const auto task = std::find_if(plan.tasks.begin(), plan.tasks.end(),
            [&](const PlanTask& candidate) { return candidate.id == id; });
        if (task != plan.tasks.end()) {
            for (const auto& dependency : task->dependencies) {
                if (ids.contains(dependency) && visit(dependency)) return true;
            }
        }
        marks[id] = 2;
        return false;
    };

    for (const auto& task : plan.tasks) {
        if (visit(task.id)) {
            result.has_cycle = true;
            break;
        }
    }
    return result;
}

} // namespace seventh::planning
