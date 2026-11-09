#include "seventh/systems/knowledge/context_manager.hpp"

namespace seventh::knowledge {

TaskContext& ContextManager::create(const TaskId& task_id) {
    auto& context = contexts_[task_id];
    context.task_id = task_id;
    return context;
}

TaskContext* ContextManager::find(const TaskId& task_id) {
    const auto found = contexts_.find(task_id);
    return found == contexts_.end() ? nullptr : &found->second;
}

const TaskContext* ContextManager::find(const TaskId& task_id) const {
    const auto found = contexts_.find(task_id);
    return found == contexts_.end() ? nullptr : &found->second;
}

bool ContextManager::erase(const TaskId& task_id) {
    return contexts_.erase(task_id) != 0;
}

} // namespace seventh::knowledge
