#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

class PlanStateManager {
public:
    [[nodiscard]] bool start(Plan& plan, const std::string& task_id) const;
    [[nodiscard]] bool complete(Plan& plan, const std::string& task_id) const;
    [[nodiscard]] bool fail(Plan& plan, const std::string& task_id) const;
};

} // namespace seventh::planning
