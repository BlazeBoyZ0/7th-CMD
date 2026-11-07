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
    if (capability->read_only && request.destructive) {
        return {PermissionDecision::Denied,
                "A read-only capability cannot authorize a destructive operation."};
    }
    if (!checker_.path_is_allowed(request.resource_path, *capability)) {
        return {PermissionDecision::Denied,
                "The target path is outside the capability's allowed locations."};
    }

    const bool confirmation_required = capability->requires_confirmation ||
        (!capability->read_only && request.destructive);
    return authorization_.authorize_confirmation(request, confirmation_required);
}

} // namespace seventh::permissions
