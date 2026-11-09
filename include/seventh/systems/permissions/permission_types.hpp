#pragma once

#include <memory>
#include <string>
#include <utility>

namespace seventh::permissions {

struct AuthorizedRoot {
    std::string canonical_path;
    int descriptor{-1};

    AuthorizedRoot(std::string path, int fd)
        : canonical_path(std::move(path)), descriptor(fd) {}
    ~AuthorizedRoot();

    AuthorizedRoot(const AuthorizedRoot&) = delete;
    AuthorizedRoot& operator=(const AuthorizedRoot&) = delete;
};

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
    std::shared_ptr<const AuthorizedRoot> authorized_root;

    PermissionResult() = default;
    PermissionResult(
        PermissionDecision permission_decision, std::string message,
        std::shared_ptr<const AuthorizedRoot> root = {})
        : decision(permission_decision), reason(std::move(message)),
          authorized_root(std::move(root)) {}
};

} // namespace seventh::permissions
