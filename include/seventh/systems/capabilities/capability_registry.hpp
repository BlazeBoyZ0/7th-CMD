#pragma once

#include "seventh/systems/capabilities/capability_definition.hpp"

#include <map>
#include <optional>
#include <string>

namespace seventh::capabilities {

class CapabilityRegistry {
public:
    bool register_capability(CapabilityDefinition definition);
    [[nodiscard]] std::optional<CapabilityDefinition> find(
        const std::string& id) const;
    [[nodiscard]] std::optional<CapabilityDefinition> for_operation(
        const std::string& operation) const;
    bool set_enabled(const std::string& id, bool enabled);

private:
    std::map<std::string, CapabilityDefinition> capabilities_;
};

} // namespace seventh::capabilities
