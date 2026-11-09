#include "seventh/engines/planning/operation_orderer.hpp"

#include <functional>
#include <map>
#include <queue>
#include <utility>
#include <vector>

namespace seventh::planning {

bool OperationOrderer::order(Plan& plan) const {
    std::map<std::string, std::size_t> index_by_id;
    for (std::size_t index = 0; index < plan.tasks.size(); ++index) {
        if (plan.tasks[index].id.empty() ||
            !index_by_id.emplace(plan.tasks[index].id, index).second) {
            return false;
        }
    }

    std::vector<std::size_t> indegree(plan.tasks.size(), 0);
    std::vector<std::vector<std::size_t>> dependents(plan.tasks.size());

    for (std::size_t task_index = 0; task_index < plan.tasks.size(); ++task_index) {
        for (const auto& dependency : plan.tasks[task_index].dependencies) {
            const auto found = index_by_id.find(dependency);
            if (found == index_by_id.end()) return false;
            ++indegree[task_index];
            dependents[found->second].push_back(task_index);
        }
    }

    // Original input position is the tie-breaker. Independent tasks retain
    // their relative order while dependencies are still enforced.
    using ReadyEntry = std::pair<std::size_t, std::size_t>;
    std::priority_queue<ReadyEntry, std::vector<ReadyEntry>,
                        std::greater<ReadyEntry>> ready;
    for (std::size_t index = 0; index < indegree.size(); ++index) {
        if (indegree[index] == 0) ready.emplace(index, index);
    }

    std::vector<PlanTask> ordered;
    ordered.reserve(plan.tasks.size());
    while (!ready.empty()) {
        const auto [original_index, current] = ready.top();
        (void)original_index;
        ready.pop();
        ordered.push_back(plan.tasks[current]);

        for (const auto dependent : dependents[current]) {
            if (--indegree[dependent] == 0) {
                ready.emplace(dependent, dependent);
            }
        }
    }

    // Never partially reorder a plan that cannot be fully scheduled.
    if (ordered.size() != plan.tasks.size()) return false;
    plan.tasks = std::move(ordered);
    return true;
}

} // namespace seventh::planning
