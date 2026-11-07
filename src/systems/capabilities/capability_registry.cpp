#include "seventh/systems/capabilities/capability_registry.hpp"

#include <algorithm>
#include <utility>

namespace seventh::capabilities {

bool CapabilityRegistry::register_capability(CapabilityDefinition definition) {
    if (definition.id.empty() || definition.operations.empty()) return false;
    capabilities_[definition.id] = std::move(definition);
    return true;
}

std::optional<CapabilityDefinition> CapabilityRegistry::find(
    const std::string& id) const {
    const auto found = capabilities_.find(id);
    if (found == capabilities_.end()) return std::nullopt;
    return found->second;
}

std::optional<CapabilityDefinition> CapabilityRegistry::for_operation(
    const std::string& operation) const {
    for (const auto& [id, capability] : capabilities_) {
        (void)id;
        if (std::find(capability.operations.begin(), capability.operations.end(),
                      operation) != capability.operations.end()) {
            return capability;
        }
    }
    return std::nullopt;
}

bool CapabilityRegistry::set_enabled(const std::string& id, bool enabled) {
    const auto found = capabilities_.find(id);
    if (found == capabilities_.end()) return false;
    found->second.enabled = enabled;
    return true;
}

} // namespace seventh::capabilities
