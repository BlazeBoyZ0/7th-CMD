#include "seventh/engines/planning/plan_state_manager.hpp"

namespace seventh::planning {
namespace {

PlanTask* find_task(Plan& plan, const std::string& id) {
    for (auto& task : plan.tasks) {
        if (task.id == id) return &task;
    }
    return nullptr;
}

} // namespace

bool PlanStateManager::start(Plan& plan, const std::string& task_id) const {
    if (!plan.valid) return false;

    auto* task = find_task(plan, task_id);
    if (!task || task->state != TaskState::New) return false;

    for (const auto& dependency : task->dependencies) {
        const auto* prerequisite = find_task(plan, dependency);
        if (!prerequisite || prerequisite->state != TaskState::Completed) {
            return false;
        }
    }

    task->state = TaskState::Processing;
    return true;
}

bool PlanStateManager::complete(Plan& plan, const std::string& task_id) const {
    if (!plan.valid) return false;
    auto* task = find_task(plan, task_id);
    if (!task || task->state != TaskState::Processing) return false;

    task->state = TaskState::Completed;
    return true;
}

bool PlanStateManager::fail(Plan& plan, const std::string& task_id) const {
    if (!plan.valid) return false;
    auto* task = find_task(plan, task_id);
    if (!task || task->state != TaskState::Processing) return false;

    task->state = TaskState::Failed;
    return true;
}

} // namespace seventh::planning
