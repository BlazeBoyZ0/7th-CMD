#include "seventh/systems/response/response_formatter.hpp"

#include <sstream>

namespace seventh::response {

std::string ResponseFormatter::format(const ResponseData& data) const {
    std::ostringstream output;
    switch (data.state) {
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
    default:
        output << "Task status: " << static_cast<int>(data.state);
        break;
    }

    if (!data.operation.empty()) output << " (" << data.operation << ")";
    if (!data.error.empty()) output << ": " << data.error;
    for (const auto& missing : data.missing_information) {
        output << "\nMissing: " << missing;
    }
    for (const auto& [key, value] : data.details) {
        output << "\n" << key << ": " << value;
    }
    if (!data.task_id.empty()) output << "\nTask ID: " << data.task_id;
    return output.str();
}

} // namespace seventh::response
