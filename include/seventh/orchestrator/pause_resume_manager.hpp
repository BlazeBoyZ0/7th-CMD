#pragma once

#include "seventh/orchestrator/task_state_manager.hpp"

namespace seventh {

class PauseResumeManager {
public:
    [[nodiscard]] bool pause_for_user(TaskStateManager& states,
                                      const TaskId& task_id) const;
    [[nodiscard]] bool pause_for_permission(TaskStateManager& states,
                                            const TaskId& task_id) const;
    [[nodiscard]] bool resume(TaskStateManager& states, const TaskId& task_id,
                              bool permission_granted = false) const;
};

} // namespace seventh
