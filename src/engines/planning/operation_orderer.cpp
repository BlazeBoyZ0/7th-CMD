#include "seventh/engines/planning/operation_orderer.hpp"

#include <map>
#include <set>
#include <utility>

namespace seventh::planning {

bool OperationOrderer::order(Plan& plan) const {
    std::map<std::string, const PlanTask*> by_id;
    std::map<std::string, std::size_t> indegree;
    std::map<std::string, std::vector<std::string>> dependents;

    for (const auto& task : plan.tasks) {
        by_id[task.id] = &task;
        indegree[task.id] = 0;
    }
    for (const auto& task : plan.tasks) {
        for (const auto& dependency : task.dependencies) {
            if (!by_id.contains(dependency)) return false;
            ++indegree[task.id];
            dependents[dependency].push_back(task.id);
        }
    }

    std::set<std::string> ready;
    for (const auto& [id, count] : indegree) {
        if (count == 0) ready.insert(id);
    }

    std::vector<PlanTask> ordered;
    ordered.reserve(plan.tasks.size());
    while (!ready.empty()) {
        const auto id = *ready.begin();
        ready.erase(ready.begin());
        ordered.push_back(*by_id.at(id));
        for (const auto& dependent : dependents[id]) {
            auto& count = indegree[dependent];
            if (--count == 0) ready.insert(dependent);
        }
    }

    if (ordered.size() != plan.tasks.size()) return false;
    plan.tasks = std::move(ordered);
    return true;
}

} // namespace seventh::planning
