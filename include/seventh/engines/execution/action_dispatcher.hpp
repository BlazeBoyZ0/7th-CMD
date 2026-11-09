#pragma once

#include "seventh/engines/execution/operation_executor.hpp"

#include <memory>
#include <vector>

namespace seventh::execution {

class ActionDispatcher {
public:
    void register_executor(std::shared_ptr<OperationExecutor> executor);
    [[nodiscard]] ExecutionResult dispatch(const ExecutionRequest& request) const;

private:
    std::vector<std::shared_ptr<OperationExecutor>> executors_;
};

} // namespace seventh::execution
