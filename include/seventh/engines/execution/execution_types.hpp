#pragma once

#include <map>
#include <memory>

namespace seventh::permissions { struct AuthorizedRoot; }
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
    std::shared_ptr<const permissions::AuthorizedRoot> authorized_root;
    std::shared_ptr<const permissions::AuthorizedRoot> authorized_destination_root;
};

struct ExecutionResult {
    ExecutionStatus status{ExecutionStatus::Failure};
    std::map<std::string, std::string> output_data;
    std::string error;
};

} // namespace seventh::execution
