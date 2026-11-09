#include "seventh/orchestrator/cancellation_handler.hpp"

namespace seventh {

bool CancellationHandler::cancel(TaskStateManager& states,
                                 const TaskId& task_id) const {
    return states.transition(task_id, TaskState::Cancelled);
}

} // namespace seventh
