#include "seventh/engines/execution/execution_tracker.hpp"

namespace seventh::execution {

bool ExecutionTracker::begin(const std::string& task_id) {
    if (task_id.empty()) return false;
    const auto [position, inserted] = states_.emplace(task_id, ExecutionState::Pending);
    if (!inserted && position->second != ExecutionState::Pending) return false;
    position->second = ExecutionState::Running;
    return true;
}

bool ExecutionTracker::complete(const std::string& task_id) {
    const auto found = states_.find(task_id);
    if (found == states_.end() || found->second != ExecutionState::Running) return false;
    found->second = ExecutionState::Completed;
    return true;
}

bool ExecutionTracker::block(const std::string& task_id) {
    const auto found = states_.find(task_id);
    if (found == states_.end() || found->second != ExecutionState::Running) return false;
    found->second = ExecutionState::Blocked;
    return true;
}

bool ExecutionTracker::fail(const std::string& task_id) {
    const auto found = states_.find(task_id);
    if (found == states_.end() || found->second != ExecutionState::Running) return false;
    found->second = ExecutionState::Failed;
    return true;
}

ExecutionState ExecutionTracker::state_of(const std::string& task_id) const {
    const auto found = states_.find(task_id);
    return found == states_.end() ? ExecutionState::Failed : found->second;
}

} // namespace seventh::execution
