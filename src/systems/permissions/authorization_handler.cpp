#include "seventh/systems/permissions/authorization_handler.hpp"

namespace seventh::permissions {

PermissionResult AuthorizationHandler::authorize_confirmation(
    const PermissionRequest& request, bool capability_requires_confirmation) const {
    if ((request.destructive || capability_requires_confirmation) &&
        !request.user_confirmed) {
        return {PermissionDecision::RequiresConfirmation,
                "This operation requires explicit user confirmation."};
    }
    return {PermissionDecision::Allowed, "Confirmation requirements are satisfied."};
}

} // namespace seventh::permissions
