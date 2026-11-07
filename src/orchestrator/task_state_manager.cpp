#include "seventh/orchestrator/task_state_manager.hpp"

namespace seventh {
namespace {

bool allowed_transition(TaskState from, TaskState to) {
    switch (from) {
    case TaskState::New:
        return to == TaskState::Understanding || to == TaskState::Failed ||
               to == TaskState::Cancelled;
    case TaskState::Understanding:
        return to == TaskState::Routing || to == TaskState::WaitingForUser ||
               to == TaskState::Failed || to == TaskState::Cancelled;
    case TaskState::Routing:
        return to == TaskState::Processing || to == TaskState::WaitingForUser ||
               to == TaskState::Failed || to == TaskState::Cancelled;
    case TaskState::Processing:
        return to == TaskState::WaitingForUser ||
               to == TaskState::WaitingForPermission || to == TaskState::Recovery ||
               to == TaskState::Completed || to == TaskState::Failed ||
               to == TaskState::Cancelled;
    case TaskState::WaitingForUser:
    case TaskState::WaitingForPermission:
        return to == TaskState::Understanding || to == TaskState::Processing ||
               to == TaskState::Cancelled || to == TaskState::Failed;
    case TaskState::Recovery:
        return to == TaskState::Processing || to == TaskState::Failed ||
               to == TaskState::Cancelled;
    case TaskState::Completed:
    case TaskState::Failed:
    case TaskState::Cancelled:
        return false;
    }
    return false;
}

} // namespace

bool TaskStateManager::create(const TaskId& task_id) {
    if (task_id.empty()) return false;
    return states_.emplace(task_id, TaskState::New).second;
}

bool TaskStateManager::transition(const TaskId& task_id, TaskState next) {
    const auto found = states_.find(task_id);
    if (found == states_.end() || !allowed_transition(found->second, next)) return false;
    found->second = next;
    return true;
}

TaskState TaskStateManager::state_of(const TaskId& task_id) const {
    const auto found = states_.find(task_id);
    return found == states_.end() ? TaskState::Failed : found->second;
}

} // namespace seventh
