#include "seventh/engines/execution/action_dispatcher.hpp"

#include <utility>

namespace seventh::execution {

void ActionDispatcher::register_executor(std::shared_ptr<OperationExecutor> executor) {
    if (!executor || executor->id().empty()) return;
    for (auto& registered : executors_) {
        if (registered->id() == executor->id()) {
            registered = std::move(executor);
            return;
        }
    }
    executors_.push_back(std::move(executor));
}

ExecutionResult ActionDispatcher::dispatch(const ExecutionRequest& request) const {
    for (const auto& executor : executors_) {
        if (executor->supports(request)) return executor->execute(request);
    }

    ExecutionResult result;
    result.status = ExecutionStatus::Unsupported;
    result.error = "No registered executor supports operation '" + request.operation + "'.";
    return result;
}

} // namespace seventh::execution
