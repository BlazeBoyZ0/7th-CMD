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
           request.operation == "search_files" ||
           request.operation == "rename_path" ||
           request.operation == "copy_file" ||
           request.operation == "delete_path" ||
           request.operation == "read_text";
}

execution::ExecutionResult LinuxOperationExecutor::execute(
    const execution::ExecutionRequest& request) {
    execution::ExecutionResult result;
    if (request.operation == "read_text") {
        const auto path = request.input_data.find("path");
        if (path == request.input_data.end() || path->second.empty()) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = "read_text requires a file path.";
            return result;
        }
        std::string read_error;
        const auto text = platform_.filesystem().read_text(path->second, read_error);
        if (!text) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = read_error;
            return result;
        }
        result.status = execution::ExecutionStatus::Success;
        result.output_data["text"] = *text;
        result.output_data["bytes"] = std::to_string(text->size());
        result.output_data["path"] = path->second;
        return result;
    }

    if (request.operation == "delete_path") {
        const auto path = request.input_data.find("path");
        if (path == request.input_data.end() || path->second.empty()) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = "delete_path requires a target path.";
            return result;
        }
        const auto removed = platform_.filesystem().remove(path->second);
        if (!removed.success) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = removed.error;
            return result;
        }
        result.status = execution::ExecutionStatus::Success;
        result.output_data["path"] = path->second;
        result.output_data["deleted"] = "true";
        return result;
    }

    if (request.operation == "rename_path" || request.operation == "copy_file") {
        const auto source = request.input_data.find("source");
        const auto destination = request.input_data.find("destination");
        if (source == request.input_data.end() || destination == request.input_data.end() ||
            source->second.empty() || destination->second.empty()) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = "A source and destination path are required.";
            return result;
        }
        const auto operation = request.operation == "rename_path"
            ? platform_.filesystem().rename(source->second, destination->second)
            : platform_.filesystem().copy(source->second, destination->second);
        if (!operation.success) {
            result.status = execution::ExecutionStatus::Failure;
            result.error = operation.error;
            return result;
        }
        result.status = execution::ExecutionStatus::Success;
        result.output_data["source"] = source->second;
        result.output_data["destination"] = destination->second;
        return result;
    }

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

    constexpr std::size_t max_displayed_matches = 50;
    std::string matches;
    std::size_t count = 0;
    std::size_t displayed = 0;
    const auto needle = query == request.input_data.end()
        ? std::string{} : lower(query->second);
    const auto type = entity_type == request.input_data.end()
        ? std::string("item") : entity_type->second;

    for (const auto& entry : entries) {
        if (type == "file" && entry.directory) continue;
        if (type == "folder" && !entry.directory) continue;
        if (!needle.empty() && lower(entry.name).find(needle) == std::string::npos) continue;
        ++count;
        if (displayed >= max_displayed_matches) continue;
        if (displayed != 0) matches += '\n';
        matches += entry.name + " | " + entry.path;
        ++displayed;
    }

    result.status = execution::ExecutionStatus::Success;
    result.output_data["count"] = std::to_string(count);
    result.output_data["displayed"] = std::to_string(displayed);
    result.output_data["truncated"] = count > displayed ? "true" : "false";
    result.output_data["matches"] = std::move(matches);
    result.output_data["directory"] = path->second;
    return result;
}

} // namespace seventh::platform::linux_backend
