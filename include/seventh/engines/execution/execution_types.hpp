#pragma once

#include <map>
#include <string>

namespace seventh::execution {

enum class ExecutionStatus {
    Success,
    NeedsPermission,
    Unsupported,
    Failure
};

enum class ExecutionState {
    Pending,
    Running,
    Completed,
    Blocked,
    Failed
};

struct ExecutionRequest {
    std::string task_id;
    std::string operation;
    std::map<std::string, std::string> input_data;
    bool requires_authorization{false};
    bool authorization_granted{false};
};

struct ExecutionResult {
    ExecutionStatus status{ExecutionStatus::Failure};
    std::map<std::string, std::string> output_data;
    std::string error;
};

} // namespace seventh::execution
