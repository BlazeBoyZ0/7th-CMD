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
        if (!tracker_.complete(request.task_id)) {
            result.status = ExecutionStatus::Failure;
            result.error = "Execution succeeded, but its tracker state could not be completed.";
        }
        break;
    case ExecutionStatus::NeedsPermission:
        if (!tracker_.block(request.task_id)) {
            result.status = ExecutionStatus::Failure;
            result.error = "Permission is required, but the tracker could not record the blocked state.";
        }
        break;
    case ExecutionStatus::Unsupported:
    case ExecutionStatus::Failure:
        if (!tracker_.fail(request.task_id)) {
            result.error += " The tracker could not record the failed state.";
        }
        break;
    }
    return result;
}

ExecutionState ExecutionEngine::state_of(const std::string& task_id) const {
    return tracker_.state_of(task_id);
}

} // namespace seventh::execution
