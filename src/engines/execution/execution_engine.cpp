#include "seventh/engines/execution/execution_engine.hpp"

#include <utility>

namespace seventh::execution {

void ExecutionEngine::register_executor(std::shared_ptr<OperationExecutor> executor) {
    dispatcher_.register_executor(std::move(executor));
}

ExecutionResult ExecutionEngine::execute(const ExecutionRequest& request) {
    ExecutionResult result;
    if (request.task_id.empty() || request.operation.empty()) {
        result.status = ExecutionStatus::Failure;
        result.error = "Execution request is missing a task identifier or operation.";
        return result;
    }

    if (request.requires_authorization && !request.authorization_granted) {
        result.status = ExecutionStatus::NeedsPermission;
        result.error = "This operation requires explicit authorization.";
        return result;
    }

    if (!tracker_.begin(request.task_id)) {
        result.status = ExecutionStatus::Failure;
        result.error = "This task is already running or has reached a terminal execution state.";
        return result;
    }

    result = dispatcher_.dispatch(request);
    switch (result.status) {
    case ExecutionStatus::Success:
        tracker_.complete(request.task_id);
        break;
    case ExecutionStatus::NeedsPermission:
        tracker_.block(request.task_id);
        break;
    case ExecutionStatus::Unsupported:
    case ExecutionStatus::Failure:
        tracker_.fail(request.task_id);
        break;
    }
    return result;
}

ExecutionState ExecutionEngine::state_of(const std::string& task_id) const {
    return tracker_.state_of(task_id);
}

} // namespace seventh::execution
