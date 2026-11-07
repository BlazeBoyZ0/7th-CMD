#include "seventh/platform/linux/linux_operation_executor.hpp"

#include <algorithm>
#include <cctype>
#include <spawn.h>
#include <string>
#include <utility>

extern char** environ;

namespace seventh::platform::linux_backend {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

} // namespace

LinuxOperationExecutor::LinuxOperationExecutor(PlatformInterface& platform)
    : platform_(platform) {}

std::string_view LinuxOperationExecutor::id() const noexcept {
    return "linux.operations";
}

bool LinuxOperationExecutor::supports(
    const execution::ExecutionRequest& request) const {
    return request.operation == "create_directory" ||
           request.operation == "launch_application" ||
           request.operation == "open_path" ||
           request.operation == "search_files";
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

    if (request.operation == "open_path") {
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

    const auto path = request.input_data.find("path");
    if (path == request.input_data.end() || path->second.empty()) {
        result.status = execution::ExecutionStatus::Failure;
        result.error = "search_files requires a directory path.";
        return result;
    }
    const auto query = request.input_data.find("query");
    const auto entity_type = request.input_data.find("entity_type");
    std::string list_error;
    const auto entries = platform_.filesystem().list(path->second, list_error);
    if (!list_error.empty()) {
        result.status = execution::ExecutionStatus::Failure;
        result.error = list_error;
        return result;
    }

    std::string matches;
    std::size_t count = 0;
    const auto needle = query == request.input_data.end()
        ? std::string{} : lower(query->second);
    const auto type = entity_type == request.input_data.end()
        ? std::string("item") : entity_type->second;

    for (const auto& entry : entries) {
        if (type == "file" && entry.directory) continue;
        if (type == "folder" && !entry.directory) continue;
        if (!needle.empty() && lower(entry.name).find(needle) == std::string::npos) continue;
        if (count != 0) matches += '\n';
        matches += entry.name + " | " + entry.path;
        ++count;
    }

    result.status = execution::ExecutionStatus::Success;
    result.output_data["count"] = std::to_string(count);
    result.output_data["matches"] = std::move(matches);
    result.output_data["directory"] = path->second;
    return result;
}

} // namespace seventh::platform::linux_backend
