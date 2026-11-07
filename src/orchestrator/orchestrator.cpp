#include "seventh/orchestrator/orchestrator.hpp"

#include <algorithm>
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

    auto& task_context = contexts_.create(request.task_id);
    task_context.values["operation"] = operation;
    for (const auto& [key, value] : request.input_data) {
        task_context.values["input." + key] = value;
    }
    for (const auto& [key, value] : request.context) {
        task_context.values["context." + key] = value;
    }

    const auto finish = [this, &outcome]() {
        outcome.state = task_states_.state_of(outcome.task_id);
        record_context(outcome);
        return outcome;
    };

    if (!task_states_.transition(request.task_id, TaskState::Understanding)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "Unable to enter the understanding state.";
        return finish();
    }

    if (!request.valid()) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "The orchestrator received an incomplete subsystem request.";
        task_states_.transition(request.task_id, TaskState::Failed);
        return finish();
    }

    if (!task_states_.transition(request.task_id, TaskState::Routing)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "Unable to enter the routing state.";
        task_states_.transition(request.task_id, TaskState::Failed);
        return finish();
    }

    const auto subsystem = router_.route(subsystems_, request);
    if (!subsystem) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "No registered subsystem can handle operation '" +
                               operation + "'.";
        task_states_.transition(request.task_id, TaskState::Failed);
        return finish();
    }

    outcome.subsystem_id = std::string(subsystem->id());
    if (!task_states_.transition(request.task_id, TaskState::Processing)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "Unable to enter the processing state.";
        task_states_.transition(request.task_id, TaskState::Failed);
        return finish();
    }

    outcome.result = result_handler_.normalize(
        dispatcher_.dispatch(*subsystem, request), request.request_id);

    const auto next = workflow_.next_state(outcome.result);
    if (!task_states_.transition(request.task_id, next)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "The workflow requested an invalid task-state transition.";
        task_states_.transition(request.task_id, TaskState::Failed);
    } else if (next == TaskState::WaitingForUser) {
        interactions_.request_missing_information(
            request.task_id, outcome.result.missing_information);
    }

    const auto final_state = task_states_.state_of(request.task_id);
    if (final_state == TaskState::WaitingForUser ||
        final_state == TaskState::WaitingForPermission ||
        final_state == TaskState::Recovery) {
        pending_tasks_[request.task_id] = PendingTask{request, subsystem};
    } else {
        pending_tasks_.erase(request.task_id);
    }

    return finish();
}

void Orchestrator::record_context(const TaskOutcome& outcome) {
    auto* context = contexts_.find(outcome.task_id);
    if (!context) return;

    context->values["task.state"] =
        std::to_string(static_cast<unsigned int>(outcome.state));
    context->values["task.subsystem"] = outcome.subsystem_id;
    context->values["result.status"] =
        std::string(to_string(outcome.result.status));
    context->values["result.error"] = outcome.result.error;
    context->unresolved_information = outcome.result.missing_information;

    for (const auto& [key, value] : outcome.result.result_data) {
        context->values["result." + key] = value;
    }
}

knowledge::KnowledgeSystem& Orchestrator::knowledge() noexcept {
    return knowledge_;
}

const knowledge::KnowledgeSystem& Orchestrator::knowledge() const noexcept {
    return knowledge_;
}

const knowledge::TaskContext* Orchestrator::context_of(
    const TaskId& task_id) const {
    return contexts_.find(task_id);
}

bool Orchestrator::clear_context(const TaskId& task_id) {
    return contexts_.erase(task_id);
}

TaskState Orchestrator::state_of(const TaskId& task_id) const {
    return task_states_.state_of(task_id);
}

bool Orchestrator::cancel(const TaskId& task_id) {
    const bool cancelled = cancellation_.cancel(task_states_, task_id);
    if (cancelled) {
        interactions_.cancel(task_id);
        pending_tasks_.erase(task_id);
        if (auto* context = contexts_.find(task_id)) {
            context->values["task.state"] =
                std::to_string(static_cast<unsigned int>(TaskState::Cancelled));
            context->unresolved_information.clear();
        }
    }
    return cancelled;
}

bool Orchestrator::resume(const TaskId& task_id, bool permission_granted) {
    const auto state = task_states_.state_of(task_id);
    if (state != TaskState::WaitingForUser &&
        state != TaskState::WaitingForPermission &&
        state != TaskState::Recovery) {
        return false;
    }
    if (state == TaskState::WaitingForUser &&
        interactions_.next_question(task_id).has_value()) {
        return false;
    }
    if (state == TaskState::WaitingForPermission && !permission_granted) {
        return false;
    }
    if (!pending_tasks_.contains(task_id)) return false;

    (void)continue_task(task_id, permission_granted);
    return true;
}

TaskOutcome Orchestrator::continue_task(
    const TaskId& task_id, bool permission_granted) {
    TaskOutcome outcome;
    outcome.task_id = task_id;
    outcome.state = task_states_.state_of(task_id);

    const auto pending = pending_tasks_.find(task_id);
    if (pending == pending_tasks_.end()) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "No resumable task is stored for this task identifier.";
        return outcome;
    }

    const auto current_state = outcome.state;
    if (current_state == TaskState::WaitingForUser &&
        interactions_.next_question(task_id).has_value()) {
        outcome.result.status = SubsystemStatus::NeedsInformation;
        outcome.result.error = "The task still has unanswered clarification requests.";
        return outcome;
    }
    if (current_state == TaskState::WaitingForPermission && !permission_granted) {
        outcome.result.status = SubsystemStatus::NeedsPermission;
        outcome.result.error = "Explicit permission is still required.";
        return outcome;
    }
    if (current_state != TaskState::WaitingForUser &&
        current_state != TaskState::WaitingForPermission &&
        current_state != TaskState::Recovery) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "The task is not in a resumable state.";
        return outcome;
    }

    if (!pause_resume_.resume(task_states_, task_id, permission_granted)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "The task could not transition back to processing.";
        return outcome;
    }

    auto request = pending->second.request;
    if (permission_granted && current_state == TaskState::WaitingForPermission) {
        request.metadata["permission_granted"] = "true";
        request.input_data["confirmed"] = "yes";
    }
    outcome.subsystem_id = std::string(pending->second.subsystem->id());
    outcome.result.request_id = request.request_id;
    outcome.result = result_handler_.normalize(
        dispatcher_.dispatch(*pending->second.subsystem, request),
        request.request_id);

    const auto next = workflow_.next_state(outcome.result);
    if (!task_states_.transition(task_id, next)) {
        outcome.result.status = SubsystemStatus::Failure;
        outcome.result.error = "The resumed workflow requested an invalid state transition.";
        task_states_.transition(task_id, TaskState::Failed);
        pending_tasks_.erase(task_id);
    } else if (next == TaskState::WaitingForUser) {
        interactions_.request_missing_information(
            task_id, outcome.result.missing_information);
        pending_tasks_[task_id].request = std::move(request);
    } else if (next == TaskState::WaitingForPermission ||
               next == TaskState::Recovery) {
        pending_tasks_[task_id].request = std::move(request);
    } else {
        pending_tasks_.erase(task_id);
    }

    outcome.state = task_states_.state_of(task_id);
    record_context(outcome);
    return outcome;
}

std::optional<knowledge::ClarificationRequest> Orchestrator::next_question(
    const TaskId& task_id) const {
    return interactions_.next_question(task_id);
}

bool Orchestrator::resolve_question(const TaskId& task_id,
                                    const std::string& field,
                                    const std::string& answer) {
    if (field.empty() || answer.empty()) return false;
    const auto pending = pending_tasks_.find(task_id);
    if (pending == pending_tasks_.end()) return false;
    if (!interactions_.resolve(task_id, field)) return false;

    pending->second.request.context["clarification." + field] = answer;
    if (auto* context = contexts_.find(task_id)) {
        context->values["answer." + field] = answer;
        context->unresolved_information.erase(
            std::remove(context->unresolved_information.begin(),
                        context->unresolved_information.end(), field),
            context->unresolved_information.end());
    }
    return true;
}

} // namespace seventh
