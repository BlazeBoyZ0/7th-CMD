#pragma once

#include "seventh/orchestrator/task_state_manager.hpp"

namespace seventh {

class CancellationHandler {
public:
    [[nodiscard]] bool cancel(TaskStateManager& states,
                              const TaskId& task_id) const;
};

} // namespace seventh
