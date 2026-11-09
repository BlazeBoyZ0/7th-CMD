#include "seventh/engines/planning/task_breakdown.hpp"

#include <algorithm>
#include <utility>

namespace seventh::planning {

std::vector<TaskDraft> TaskBreakdown::normalize(
    const std::vector<TaskDraft>& tasks) const {
    std::vector<TaskDraft> result;
    result.reserve(tasks.size());

    for (const auto& task : tasks) {
        TaskDraft normalized = task;
        StringList unique_dependencies;
        unique_dependencies.reserve(task.dependencies.size());

        for (const auto& dependency : task.dependencies) {
            // Preserve empty entries: validation must report malformed input
            // rather than silently deleting an intended dependency.
            if (std::find(unique_dependencies.begin(), unique_dependencies.end(),
                          dependency) == unique_dependencies.end()) {
                unique_dependencies.push_back(dependency);
            }
        }

        normalized.dependencies = std::move(unique_dependencies);
        result.push_back(std::move(normalized));
    }

    return result;
}

} // namespace seventh::planning
