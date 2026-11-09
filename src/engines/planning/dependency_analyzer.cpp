#include "seventh/engines/planning/dependency_analyzer.hpp"

#include <functional>
#include <map>
#include <queue>
#include <utility>
#include <vector>

namespace seventh::planning {

DependencyAnalysis DependencyAnalyzer::analyze(const Plan& plan) const {
    DependencyAnalysis result;
    std::map<std::string, std::size_t> index_by_id;

    for (std::size_t index = 0; index < plan.tasks.size(); ++index) {
        const auto& id = plan.tasks[index].id;
        const auto [iterator, inserted] = index_by_id.emplace(id, index);
        (void)iterator;
        if (!inserted) result.has_duplicate_task_ids = true;
    }

    if (result.has_duplicate_task_ids) return result;

    std::vector<std::size_t> indegree(plan.tasks.size(), 0);
    std::vector<std::vector<std::size_t>> dependents(plan.tasks.size());

    for (std::size_t task_index = 0; task_index < plan.tasks.size(); ++task_index) {
        for (const auto& dependency : plan.tasks[task_index].dependencies) {
            const auto found = index_by_id.find(dependency);
            if (found == index_by_id.end()) {
                result.missing_dependencies.push_back(dependency);
                continue;
            }
            ++indegree[task_index];
            dependents[found->second].push_back(task_index);
        }
    }

    // Kahn's algorithm avoids recursive DFS stack growth on long user plans.
    std::queue<std::size_t> ready;
    for (std::size_t index = 0; index < indegree.size(); ++index) {
        if (indegree[index] == 0) ready.push(index);
    }

    std::size_t processed = 0;
    while (!ready.empty()) {
        const auto current = ready.front();
        ready.pop();
        ++processed;

        for (const auto dependent : dependents[current]) {
            if (--indegree[dependent] == 0) ready.push(dependent);
        }
    }

    result.has_cycle = processed != plan.tasks.size();
    return result;
}

} // namespace seventh::planning
