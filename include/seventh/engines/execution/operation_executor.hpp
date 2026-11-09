#pragma once

#include "seventh/engines/execution/execution_types.hpp"

#include <string_view>

namespace seventh::execution {

class OperationExecutor {
public:
    virtual ~OperationExecutor() = default;
    [[nodiscard]] virtual std::string_view id() const noexcept = 0;
    [[nodiscard]] virtual bool supports(const ExecutionRequest& request) const = 0;
    [[nodiscard]] virtual ExecutionResult execute(const ExecutionRequest& request) = 0;
};

} // namespace seventh::execution
