#pragma once

#include "seventh/systems/capabilities/capability_definition.hpp"

#include <string>

namespace seventh::permissions {

class PermissionChecker {
public:
    [[nodiscard]] bool path_is_allowed(
        const std::string& path,
        const capabilities::CapabilityDefinition& capability) const;
};

} // namespace seventh::permissions
