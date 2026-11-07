#include "seventh/engines/planning/plan_optimizer.hpp"

namespace seventh::planning {

void PlanOptimizer::optimize(Plan& plan) const {
    // Preserve the validated dependency order. Future optimizations must not
    // reorder dependent tasks or weaken execution constraints.
    (void)plan;
}

} // namespace seventh::planning
