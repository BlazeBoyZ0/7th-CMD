#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

class TaskBreakdown {
public:
    [[nodiscard]] std::vector<TaskDraft> normalize(const std::vector<TaskDraft>& tasks) const;
};

} // namespace seventh::planning
