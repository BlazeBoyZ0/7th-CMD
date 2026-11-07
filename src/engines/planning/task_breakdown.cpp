#include "seventh/engines/planning/task_breakdown.hpp"

namespace seventh::planning {

std::vector<TaskDraft> TaskBreakdown::normalize(const std::vector<TaskDraft>& tasks) const {
    std::vector<TaskDraft> result;
    result.reserve(tasks.size());
    for (const auto& task : tasks) {
        if (!task.operation.empty()) result.push_back(task);
    }
    return result;
}

} // namespace seventh::planning
