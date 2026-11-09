#include "seventh/systems/response/response_formatter.hpp"
#include "seventh/cli/terminal_output.hpp"

#include <sstream>

namespace seventh::response {

std::string ResponseFormatter::format(const ResponseData& data) const {
    std::ostringstream output;
    switch (data.state) {
    case TaskState::New:
        output << "Task created";
        break;
    case TaskState::Understanding:
        output << "Understanding the request";
        break;
    case TaskState::Routing:
        output << "Selecting the required subsystem";
        break;
    case TaskState::Processing:
        output << "Processing the request";
        break;
    case TaskState::Recovery:
        output << "Recovery is required before the task can continue";
        break;
    case TaskState::Completed:
        output << "Completed";
        break;
    case TaskState::WaitingForUser:
        output << "I need more information";
        break;
    case TaskState::WaitingForPermission:
        output << "Permission is required";
        break;
    case TaskState::Failed:
        output << "The task failed";
        break;
    case TaskState::Cancelled:
        output << "The task was cancelled";
        break;
    }

    if (!data.operation.empty()) output << " (" << cli::escape_terminal_text(data.operation) << ")";
    if (!data.error.empty()) output << ": " << cli::escape_terminal_text(data.error);
    for (const auto& missing : data.missing_information) {
        output << "\nMissing: " << cli::escape_terminal_text(missing);
    }
    for (const auto& [key, value] : data.details) {
        output << "\n" << cli::escape_terminal_text(key) << ": " << cli::escape_terminal_text(value);
    }
    if (!data.task_id.empty()) output << "\nTask ID: " << cli::escape_terminal_text(data.task_id);
    return output.str();
}

} // namespace seventh::response
