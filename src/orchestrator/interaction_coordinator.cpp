#include "seventh/orchestrator/interaction_coordinator.hpp"

namespace seventh {

void InteractionCoordinator::request_missing_information(
    const TaskId& task_id, const StringList& missing_information) {
    for (const auto& field : missing_information) {
        knowledge::ClarificationRequest request;
        request.task_id = task_id;
        request.required_field = field;
        request.question = field.find("choice") != std::string::npos
            ? "Choose one of the listed options by number, name, or full path."
            : "Please provide " + field + ".";
        clarifications_.request(request);
    }
}

std::optional<knowledge::ClarificationRequest>
InteractionCoordinator::next_question(const TaskId& task_id) const {
    return clarifications_.next(task_id);
}

bool InteractionCoordinator::resolve(const TaskId& task_id,
                                     const std::string& field) {
    return clarifications_.resolve(task_id, field);
}

void InteractionCoordinator::cancel(const TaskId& task_id) {
    clarifications_.cancel_task(task_id);
}

} // namespace seventh
