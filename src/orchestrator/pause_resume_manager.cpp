#include "seventh/orchestrator/pause_resume_manager.hpp"

namespace seventh {

bool PauseResumeManager::pause_for_user(TaskStateManager& states,
                                        const TaskId& task_id) const {
    return states.transition(task_id, TaskState::WaitingForUser);
}

bool PauseResumeManager::pause_for_permission(TaskStateManager& states,
                                              const TaskId& task_id) const {
    return states.transition(task_id, TaskState::WaitingForPermission);
}

bool PauseResumeManager::resume(TaskStateManager& states,
                                const TaskId& task_id) const {
    const auto state = states.state_of(task_id);
    if (state != TaskState::WaitingForUser &&
        state != TaskState::WaitingForPermission &&
        state != TaskState::Recovery) return false;
    return states.transition(task_id, TaskState::Processing);
}

} // namespace seventh
