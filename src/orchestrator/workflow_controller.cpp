#include "seventh/orchestrator/workflow_controller.hpp"

namespace seventh {

TaskState WorkflowController::next_state(const SubsystemResult& result) const noexcept {
    switch (result.status) {
    case SubsystemStatus::Success: return TaskState::Completed;
    case SubsystemStatus::NeedsInformation: return TaskState::WaitingForUser;
    case SubsystemStatus::NeedsPermission: return TaskState::WaitingForPermission;
    case SubsystemStatus::NewRequirements:
    case SubsystemStatus::Uncertain: return TaskState::Recovery;
    case SubsystemStatus::Failure: return TaskState::Failed;
    }
    return TaskState::Failed;
}

} // namespace seventh
