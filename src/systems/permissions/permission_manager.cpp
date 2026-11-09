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
    if (capability->read_only &&
        (request.destructive || request.modifies_state)) {
        return {PermissionDecision::Denied,
                "A read-only capability cannot authorize an operation that changes system state."};
    }

    std::shared_ptr<const AuthorizedRoot> authorized_root;
    if (!checker_.open_authorized_root(
            request.resource_path, *capability, authorized_root)) {
        return {PermissionDecision::Denied,
                "The target path is outside the capability's allowed locations."};
    }

    const bool confirmation_required = capability->requires_confirmation ||
        (!capability->read_only && request.destructive);
    auto confirmation = authorization_.authorize_confirmation(
        request, confirmation_required);
    confirmation.authorized_root = std::move(authorized_root);
    return confirmation;
}

} // namespace seventh::permissions
