#pragma once

#include "seventh/systems/capabilities/capability_registry.hpp"
#include "seventh/systems/permissions/authorization_handler.hpp"
#include "seventh/systems/permissions/permission_checker.hpp"

namespace seventh::permissions {

class PermissionManager {
public:
    explicit PermissionManager(const capabilities::CapabilityRegistry& registry);
    [[nodiscard]] PermissionResult authorize(const PermissionRequest& request) const;

private:
    const capabilities::CapabilityRegistry& registry_;
    PermissionChecker checker_;
    AuthorizationHandler authorization_;
};

} // namespace seventh::permissions
