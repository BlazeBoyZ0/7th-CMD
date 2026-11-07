#include "seventh/systems/permissions/permission_manager.hpp"

#include <algorithm>

namespace seventh::permissions {

PermissionManager::PermissionManager(
    const capabilities::CapabilityRegistry& registry)
    : registry_(registry) {}

PermissionResult PermissionManager::authorize(
    const PermissionRequest& request) const {
    const auto capability = request.capability_id.empty()
        ? registry_.for_operation(request.operation)
        : registry_.find(request.capability_id);

    if (!capability) {
        return {PermissionDecision::Denied, "The requested capability is not registered."};
    }
    if (!capability->enabled) {
        return {PermissionDecision::Denied, "The requested capability is disabled."};
    }
    if (std::find(capability->operations.begin(), capability->operations.end(),
                  request.operation) == capability->operations.end()) {
        return {PermissionDecision::Denied, "The capability does not permit this operation."};
    }
    if (!checker_.path_is_allowed(request.resource_path, *capability)) {
        return {PermissionDecision::Denied,
                "The target path is outside the capability's allowed locations."};
    }
    if (!capability->read_only && request.destructive && !request.user_confirmed) {
        return {PermissionDecision::RequiresConfirmation,
                "A modifying operation requires explicit user confirmation."};
    }
    return authorization_.authorize_confirmation(
        request, capability->requires_confirmation);
}

} // namespace seventh::permissions
