#include "seventh/platform/linux/linux_operation_executor.hpp"

#include <spawn.h>
#include <string>

extern char** environ;

namespace seventh::platform::linux_backend {

LinuxOperationExecutor::LinuxOperationExecutor(PlatformInterface& platform)
    : platform_(platform) {}

std::string_view LinuxOperationExecutor::id() const noexcept {
    return "linux.operations";
}

bool LinuxOperationExecutor::supports(
    const execution::ExecutionRequest& request) const {
    return request.operation == "create_directory" ||
           request.operation == "launch_application" ||
           request.operation == "open_path";
}

execution::ExecutionResult LinuxOperationExecutor::execute(
    const execution::ExecutionRequest& request) {
    execution::ExecutionResult result;
    if (request.operation == "create_directory") {
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

    if (request.operation == "launch_application") {
        const auto id = request.input_data.find("application_id");
        if (id == request.input_data.end() || id->second.empty()) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = "launch_application requires an application ID.";
            return result;
        }
        const auto application = platform_.applications().find(id->second);
        if (!application) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = "The selected application is no longer available.";
            return result;
        }
        const auto launched = platform_.applications().launch(*application);
        if (!launched.success) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = launched.error;
            return result;
        }
        result.status = execution::ExecutionStatus::Success;
        result.output_data["started"] = "true";
        result.output_data["application"] = application->name;
        return result;
    }

    const auto path = request.input_data.find("path");
    if (path == request.input_data.end() || path->second.empty()) {
        result.status = execution::ExecutionStatus::Failure;
        result.error = "open_path requires a target path.";
        return result;
    }
    pid_t child = 0;
    char* const arguments[] = {
        const_cast<char*>("xdg-open"),
        const_cast<char*>(path->second.c_str()),
        nullptr
    };
    const int status = posix_spawnp(&child, "xdg-open", nullptr, nullptr,
                                    arguments, environ);
    if (status != 0) {
        result.status = execution::ExecutionStatus::Failure;
        result.error = "Unable to start xdg-open (error " +
                       std::to_string(status) + ").";
        return result;
    }
    result.status = execution::ExecutionStatus::Success;
    result.output_data["started"] = "true";
    result.output_data["path"] = path->second;
    return result;
}

} // namespace seventh::platform::linux_backend
