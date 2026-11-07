#pragma once

#include <string>

namespace seventh::permissions {

enum class PermissionDecision {
    Allowed,
    Denied,
    RequiresConfirmation
};

struct PermissionRequest {
    std::string capability_id;
    std::string operation;
    std::string resource_path;
    bool destructive{false};
    bool modifies_state{false};
    bool user_confirmed{false};
};

struct PermissionResult {
    PermissionDecision decision{PermissionDecision::Denied};
    std::string reason;
};

} // namespace seventh::permissions
