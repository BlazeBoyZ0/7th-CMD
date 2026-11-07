#include "seventh/systems/capabilities/capability_registry.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <algorithm>
#include <set>
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

bool CapabilityRegistry::load_from_file(const std::string& path,
                                        std::string& error) {
    utilities::JsonLoader loader;
    const auto parsed = loader.load_file(path, error);
    if (!parsed) return false;

    const auto* root = parsed->as_object();
    if (!root) {
        error = "The capability configuration root must be an object.";
        return false;
    }

    const auto version_entry = root->find("schema_version");
    const auto* version = version_entry == root->end()
        ? nullptr : version_entry->second.as_number();
    if (!version || *version != 1.0) {
        error = "The capability configuration has an unsupported schema version.";
        return false;
    }

    const auto capabilities_entry = root->find("capabilities");
    const auto* definitions = capabilities_entry == root->end()
        ? nullptr : capabilities_entry->second.as_array();
    if (!definitions) {
        error = "The capability configuration must contain a capabilities array.";
        return false;
    }

    std::vector<CapabilityDefinition> loaded;
    std::set<std::string> identifiers;
    for (std::size_t index = 0; index < definitions->size(); ++index) {
        const auto* object = (*definitions)[index].as_object();
        if (!object) {
            error = "Capability entry " + std::to_string(index) +
                    " must be an object.";
            return false;
        }

        const auto get = [&](const std::string& key) -> const utilities::JsonValue* {
            const auto found = object->find(key);
            return found == object->end() ? nullptr : &found->second;
        };

        const auto* id_value = get("id");
        const auto* operations_value = get("operations");
        const auto* enabled_value = get("enabled");
        const auto* read_only_value = get("read_only");
        const auto* id = id_value ? id_value->as_string() : nullptr;
        const auto* operations = operations_value ? operations_value->as_array() : nullptr;
        const auto* enabled = enabled_value ? enabled_value->as_bool() : nullptr;
        const auto* read_only = read_only_value ? read_only_value->as_bool() : nullptr;

        if (!id || id->empty() || !operations || operations->empty() ||
            !enabled || !read_only || !identifiers.insert(*id).second) {
            error = "Capability entry " + std::to_string(index) +
                    " has missing, invalid, or duplicate required fields.";
            return false;
        }

        CapabilityDefinition definition;
        definition.id = *id;
        definition.enabled = *enabled;
        definition.read_only = *read_only;

        if (const auto* description_value = get("description")) {
            const auto* description = description_value->as_string();
            if (!description) {
                error = "Capability description must be a string.";
                return false;
            }
            definition.description = *description;
        }

        for (const auto& operation_value : *operations) {
            const auto* operation = operation_value.as_string();
            if (!operation || operation->empty()) {
                error = "Capability operations must be non-empty strings.";
                return false;
            }
            definition.operations.push_back(*operation);
        }

        if (const auto* roots_value = get("allowed_roots")) {
            const auto* roots = roots_value->as_array();
            if (!roots) {
                error = "Capability allowed_roots must be an array.";
                return false;
            }
            for (const auto& root_value : *roots) {
                const auto* allowed_root = root_value.as_string();
                if (!allowed_root || allowed_root->empty()) {
                    error = "Capability allowed_roots entries must be non-empty strings.";
                    return false;
                }
                definition.allowed_roots.push_back(*allowed_root);
            }
        }

        if (const auto* confirmation_value = get("requires_confirmation")) {
            const auto* confirmation = confirmation_value->as_bool();
            if (!confirmation) {
                error = "Capability requires_confirmation must be a boolean.";
                return false;
            }
            definition.requires_confirmation = *confirmation;
        }

        loaded.push_back(std::move(definition));
    }

    // Apply only after the entire file has passed validation.
    for (auto& definition : loaded) {
        register_capability(std::move(definition));
    }
    error.clear();
    return true;
}

bool CapabilityRegistry::set_enabled(const std::string& id, bool enabled) {
    const auto found = capabilities_.find(id);
    if (found == capabilities_.end()) return false;
    found->second.enabled = enabled;
    return true;
}

} // namespace seventh::capabilities
