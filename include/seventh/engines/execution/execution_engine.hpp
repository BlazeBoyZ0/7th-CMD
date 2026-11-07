#pragma once

#include "seventh/engines/execution/action_dispatcher.hpp"
#include "seventh/engines/execution/execution_tracker.hpp"

namespace seventh::execution {

class ExecutionEngine {
public:
    void register_executor(std::shared_ptr<OperationExecutor> executor);
    [[nodiscard]] ExecutionResult execute(const ExecutionRequest& request);
    [[nodiscard]] ExecutionState state_of(const std::string& task_id) const;

private:
    ActionDispatcher dispatcher_;
    ExecutionTracker tracker_;
};

} // namespace seventh::execution
