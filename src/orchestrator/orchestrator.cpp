#include "seventh/orchestrator/orchestrator.hpp"

#include <utility>

namespace seventh {

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

TaskOutcome Orchestrator::execute(
    const std::string& operation,
    std::map<std::string, std::string> input_data,
    std::map<std::string, std::string> context,
    StringList constraints) {
    const auto request = request_manager_.create(
        operation, std::move(input_data), std::move(context), std::move(constraints));

    TaskOutcome outcome;
    outcome.task_id = request.task_id;
    outcome.result.request_id = request.request_id;
    task_states_.create(request.task_id);

    if (!task_states_.transition(request.task_id, TaskState::Understanding)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "Unable to enter the understanding state.";
        outcome.state = task_states_.state_of(request.task_id);
        return outcome;
    }

    if (!request.valid()) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "The orchestrator received an incomplete subsystem request.";
        task_states_.transition(request.task_id, TaskState::Failed);
        outcome.state = task_states_.state_of(request.task_id);
        return outcome;
    }

    task_states_.transition(request.task_id, TaskState::Routing);
    const auto subsystem = router_.route(subsystems_, request);
    if (!subsystem) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "No registered subsystem can handle operation '" +
                               operation + "'.";
        task_states_.transition(request.task_id, TaskState::Failed);
        outcome.state = task_states_.state_of(request.task_id);
        return outcome;
    }

    outcome.subsystem_id = std::string(subsystem->id());
    task_states_.transition(request.task_id, TaskState::Processing);
    outcome.result = result_handler_.normalize(
        dispatcher_.dispatch(*subsystem, request), request.request_id);

    const auto next = workflow_.next_state(outcome.result);
    task_states_.transition(request.task_id, next);
    if (next == TaskState::WaitingForUser) {
        interactions_.request_missing_information(
            request.task_id, outcome.result.missing_information);
    }

    outcome.state = task_states_.state_of(request.task_id);
    return outcome;
}

TaskState Orchestrator::state_of(const TaskId& task_id) const {
    return task_states_.state_of(task_id);
}

bool Orchestrator::cancel(const TaskId& task_id) {
    const bool cancelled = cancellation_.cancel(task_states_, task_id);
    if (cancelled) interactions_.cancel(task_id);
    return cancelled;
}

bool Orchestrator::resume(const TaskId& task_id) {
    return pause_resume_.resume(task_states_, task_id);
}

std::optional<knowledge::ClarificationRequest> Orchestrator::next_question(
    const TaskId& task_id) const {
    return interactions_.next_question(task_id);
}

bool Orchestrator::resolve_question(const TaskId& task_id,
                                    const std::string& field) {
    return interactions_.resolve(task_id, field);
}

} // namespace seventh
