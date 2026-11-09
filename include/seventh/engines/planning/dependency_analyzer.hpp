#pragma once

#include "seventh/engines/planning/planning_types.hpp"

namespace seventh::planning {

struct DependencyAnalysis {
    StringList missing_dependencies;
    bool has_cycle{false};
    bool has_duplicate_task_ids{false};
};

class DependencyAnalyzer {
public:
    [[nodiscard]] DependencyAnalysis analyze(const Plan& plan) const;
};

} // namespace seventh::planning
