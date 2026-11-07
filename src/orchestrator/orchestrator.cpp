#include "seventh/orchestrator/orchestrator.hpp"

#include "seventh/core/identifiers.hpp"

#include <utility>

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

SubsystemResult failure_result(const RequestId& request_id, std::string message) {
    SubsystemResult result;
    result.request_id = request_id;
    result.status = SubsystemStatus::Failure;
    result.error = std::move(message);
    return result;
}

} // namespace

void Orchestrator::register_subsystem(std::shared_ptr<Subsystem> subsystem) {
    if (!subsystem || subsystem->id().empty()) return;
    for (auto& registered : subsystems_) {
        if (registered->id() == subsystem->id()) {
            registered = std::move(subsystem);
            return;
        }
    }
    subsystems_.push_back(std::move(subsystem));
}

std::shared_ptr<Subsystem> Orchestrator::select_subsystem(
    const SubsystemRequest& request) const {
    for (const auto& subsystem : subsystems_) {
        if (subsystem->can_handle(request)) return subsystem;
    }
    return {};
}

bool Orchestrator::transition(const TaskId& task_id, TaskState next) {
    const auto found = task_states_.find(task_id);
    if (found == task_states_.end() || !allowed_transition(found->second, next)) {
        return false;
    }
    found->second = next;
    return true;
}

TaskState Orchestrator::state_of(const TaskId& task_id) const {
    const auto found = task_states_.find(task_id);
    return found == task_states_.end() ? TaskState::Failed : found->second;
}

TaskOutcome Orchestrator::execute(
    const std::string& operation,
    std::map<std::string, std::string> input_data,
    std::map<std::string, std::string> context,
    StringList constraints) {
    SubsystemRequest request;
    request.request_id = make_identifier("request");
    request.task_id = make_identifier("task");
    request.operation = operation;
    request.input_data = std::move(input_data);
    request.context = std::move(context);
    request.constraints = std::move(constraints);

    TaskOutcome outcome;
    outcome.task_id = request.task_id;
    outcome.result.request_id = request.request_id;
    task_states_.emplace(request.task_id, TaskState::New);

    transition(request.task_id, TaskState::Understanding);
    if (!request.valid()) {
        outcome.result = failure_result(request.request_id,
            "The orchestrator received an incomplete subsystem request.");
        transition(request.task_id, TaskState::Failed);
        outcome.state = state_of(request.task_id);
        return outcome;
    }

    if (!transition(request.task_id, TaskState::Routing)) {
        outcome.result = failure_result(request.request_id,
            "Invalid task lifecycle transition while routing.");
        transition(request.task_id, TaskState::Failed);
        outcome.state = state_of(request.task_id);
        return outcome;
    }

    const auto subsystem = select_subsystem(request);
    if (!subsystem) {
        outcome.result = failure_result(request.request_id,
            "No registered subsystem can handle operation '" + operation + "'.");
        transition(request.task_id, TaskState::Failed);
        outcome.state = state_of(request.task_id);
        return outcome;
    }

    outcome.subsystem_id = std::string(subsystem->id());
    transition(request.task_id, TaskState::Processing);
    outcome.result = subsystem->handle(request);
    if (outcome.result.request_id.empty()) {
        outcome.result.request_id = request.request_id;
    }

    switch (outcome.result.status) {
    case SubsystemStatus::Success:
        transition(request.task_id, TaskState::Completed);
        break;
    case SubsystemStatus::NeedsInformation:
        transition(request.task_id, TaskState::WaitingForUser);
        break;
    case SubsystemStatus::NeedsPermission:
        transition(request.task_id, TaskState::WaitingForPermission);
        break;
    case SubsystemStatus::NewRequirements:
    case SubsystemStatus::Uncertain:
        transition(request.task_id, TaskState::Recovery);
        break;
    case SubsystemStatus::Failure:
        transition(request.task_id, TaskState::Failed);
        break;
    }

    outcome.state = state_of(request.task_id);
    return outcome;
}

} // namespace seventh
