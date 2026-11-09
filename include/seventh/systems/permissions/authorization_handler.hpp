#pragma once

#include "seventh/systems/permissions/permission_types.hpp"

namespace seventh::permissions {

class AuthorizationHandler {
public:
    [[nodiscard]] PermissionResult authorize_confirmation(
        const PermissionRequest& request, bool capability_requires_confirmation) const;
};

} // namespace seventh::permissions
