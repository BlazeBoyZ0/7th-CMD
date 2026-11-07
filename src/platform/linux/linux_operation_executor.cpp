#include "seventh/platform/linux/linux_operation_executor.hpp"

namespace seventh::platform::linux_backend {

LinuxOperationExecutor::LinuxOperationExecutor(PlatformInterface& platform)
    : platform_(platform) {}

std::string_view LinuxOperationExecutor::id() const noexcept {
    return "linux.operations";
}

bool LinuxOperationExecutor::supports(
    const execution::ExecutionRequest& request) const {
    return request.operation == "create_directory";
}

execution::ExecutionResult LinuxOperationExecutor::execute(
    const execution::ExecutionRequest& request) {
    execution::ExecutionResult result;
    const auto path = request.input_data.find("path");
    if (path == request.input_data.end() || path->second.empty()) {
        result.status = execution::ExecutionStatus::Failure;
        result.error = "create_directory requires an absolute path.";
        return result;
    }

    const auto operation = platform_.filesystem().create_directory(path->second);
    if (!operation.success) {
        result.status = execution::ExecutionStatus::Failure;
        result.error = operation.error;
        return result;
    }

    result.status = execution::ExecutionStatus::Success;
    result.output_data["path"] = path->second;
    result.output_data["created"] = "true";
    return result;
}

} // namespace seventh::platform::linux_backend
