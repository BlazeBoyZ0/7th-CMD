#pragma once

#include "seventh/engines/execution/operation_executor.hpp"
#include "seventh/platform/platform_interface.hpp"

namespace seventh::platform::linux_backend {

class LinuxOperationExecutor final : public execution::OperationExecutor {
public:
    explicit LinuxOperationExecutor(PlatformInterface& platform);

    [[nodiscard]] std::string_view id() const noexcept override;
    [[nodiscard]] bool supports(
        const execution::ExecutionRequest& request) const override;
    [[nodiscard]] execution::ExecutionResult execute(
        const execution::ExecutionRequest& request) override;

private:
    PlatformInterface& platform_;
};

} // namespace seventh::platform::linux_backend
