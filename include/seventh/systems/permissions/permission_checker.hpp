#pragma once

#include "seventh/systems/capabilities/capability_definition.hpp"
#include "seventh/systems/permissions/permission_types.hpp"

#include <memory>
#include <string>

namespace seventh::permissions {

class PermissionChecker {
public:
    [[nodiscard]] bool open_authorized_root(
        const std::string& path,
        const capabilities::CapabilityDefinition& capability,
        std::shared_ptr<const AuthorizedRoot>& authorized_root) const;
};

} // namespace seventh::permissions
